package partition

import (
	"crypto/sha256"
	"encoding/hex"

	"github.com/pkg/errors"
	"log"
)

func init() {
	registerPartitionFactory(LFMETA_SHA256, lfmetaSha256ParitionFactory)
}

var lfmetaSha256ParitionFactory = func(args PartitionFactoryArgs) Partition {
	return &LFMetaSha256Partition{
		Name:     args.PInfo.Name,
		Data:     args.Data,
		Offset:   args.PInfo.Offset,
		Checksum: args.PInfo.Checksum,
	}
}

type LFMetaSha256Partition struct {
	Name     string
	Data     []byte
	Offset   uint32
	Checksum string
}

func (p *LFMetaSha256Partition) GetName() string {
	return p.Name
}

func (p *LFMetaSha256Partition) GetSize() uint32 {
	return uint32(len(p.Data))
}

func (p *LFMetaSha256Partition) GetType() PartitionConfigType {
	return LFMETA_SHA256
}

func (p *LFMetaSha256Partition) Validate() error {
	hash := sha256.Sum256(p.Data)
	hashStr := hex.EncodeToString(hash[:])

	if hashStr != p.Checksum {
		return errors.Errorf("'%v' partition SHA256 (%v) does not match expected (%v)",
			p.Name, hashStr, p.Checksum)
	}

	log.Printf("'%v' partition SHA256 (%v) OK", p.Name, hashStr)
	return nil
}
