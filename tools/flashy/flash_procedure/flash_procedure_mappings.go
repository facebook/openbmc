// @generated
// This file is auto-generated. Do not modify directly.

package flash_procedure

import (
	"github.com/facebook/openbmc/tools/flashy/lib/flash"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
)

// platform build name -> flash procedure entry point mapping
// see flash_procedure/platform_registry.go for platform-specific overrides
var GeneratedFlashProcedureMappings = map[string]func(step.StepParams) step.StepExitError{
	"acctonbmc":        flash.FlashCp,
	"angelslanding":    flash.FlashCpVboot,
	"bletchley15":      flash.FlashCp,
	"bletchley":        flash.FlashCp,
	"catalina":         flash.FlashCp,
	"celesticabmc":     flash.FlashCp,
	"clearcreek":       flash.FlashCpVboot,
	"clemente":         flash.FlashCp,
	"elbert":           flash.FlashCp,
	"emeraldpools":     flash.FlashCpVboot,
	"fbdarwin":         flash.FlashCp,
	"fbgp2":            flash.FlashCpVboot,
	"fbtp":             flash.FlashCpVboot,
	"fbttn":            flash.FlashCpVboot,
	"fby2":             flash.FlashCpVboot,
	"fby35":            flash.FlashCpVboot,
	"fby3":             flash.FlashCpVboot,
	"fuji":             flash.FlashCp,
	"grandcanyon":      flash.FlashCpVboot,
	"grandteton":       flash.FlashCpVboot,
	"greatlakes":       flash.FlashCpVboot,
	"gtartemis":        flash.FlashCpVboot,
	"halfdome":         flash.FlashCpVboot,
	"harma":            flash.FlashCp,
	"inspirationpoint": flash.FlashCpVboot,
	"janga":            flash.FlashCp,
	"javaisland":       flash.FlashCpVboot,
	"lightning":        flash.FlashCp,
	"meru":             flash.FlashCp,
	"minerva":          flash.FlashCp,
	"minipack3n":       flash.FlashCp,
	"minipack":         flash.FlashCp,
	"montblanc":        flash.FlashCp,
	"morgan800cc":      flash.FlashCp,
	"northdome":        flash.FlashCpVboot,
	"santabarbara":     flash.FlashCp,
	"tahan":            flash.FlashCp,
	"ventura":          flash.FlashCp,
	"wedge100":         flash.FlashCp,
	"wedge400":         flash.FlashCp,
	"yamp":             flash.FlashCp,
	"yosemite4":        flash.FlashCp,
	"yosemite5":        flash.FlashCp,
	"yosemite":         flash.FlashCp,
}
