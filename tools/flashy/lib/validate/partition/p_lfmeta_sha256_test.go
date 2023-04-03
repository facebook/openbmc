package partition

import (
	"reflect"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

func TestLFMetaSha256Partition(t *testing.T) {
	exampleData := []byte("this is a test")
	exampleDataSha256Sum := "2e99758548972a8e8822ad47fa1017ff72f06f3ff6a016851f45c398732bc50c"

	args := PartitionFactoryArgs{
		Data: exampleData,
		PInfo: PartitionConfigInfo{
			Name:     "foo",
			Offset:   1024,
			Size:     4,
			Type:     LFMETA_SHA256,
			Checksum: exampleDataSha256Sum,
		},
	}
	wantP := &LFMetaSha256Partition{
		Name:     "foo",
		Offset:   1024,
		Data:     exampleData,
		Checksum: exampleDataSha256Sum,
	}

	p := lfmetaSha256ParitionFactory(args)
	if !reflect.DeepEqual(wantP, p) {
		t.Errorf("partition: want '%v' got '%v'", wantP, p)
	}
	gotName := p.GetName()
	if "foo" != gotName {
		t.Errorf("name: want '%v' got '%v'", "foo", gotName)
	}
	gotSize := p.GetSize()
	if uint32(14) != gotSize {
		t.Errorf("size: want '%v' got '%v'", 4, gotSize)
	}
	gotValidatorType := p.GetType()
	if gotValidatorType != LFMETA_SHA256 {
		t.Errorf("validator type: want '%v' got '%v'",
			LFMETA_SHA256, gotValidatorType)
	}
}

// Validate tests
func TestLFMetaSHA256Validate(t *testing.T) {
	exampleData := []byte("this is a test")
	exampleDataSha256Sum := "2e99758548972a8e8822ad47fa1017ff72f06f3ff6a016851f45c398732bc50c"

	cases := []struct {
		name     string
		checksum string
		want     error
	}{
		{
			name:     "validation succeeded",
			checksum: exampleDataSha256Sum,
			want:     nil,
		},
		{
			name:     "validation failed",
			checksum: "UWU",
			want: errors.Errorf("'foo' partition SHA256 "+
				"(%v) does not match expected (UWU)", exampleDataSha256Sum),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			p := &LFMetaSha256Partition{
				Name:     "foo",
				Offset:   1024,
				Data:     exampleData,
				Checksum: tc.checksum,
			}
			got := p.Validate()
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}
