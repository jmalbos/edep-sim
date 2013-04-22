#include "CaptDriftRegionBuilder.hh"
#include "CaptWirePlaneBuilder.hh"
#include "DSimBuilder.hh"

#include <DSimLog.hh>

#include <globals.hh>
#include <G4Material.hh>
#include <G4LogicalVolume.hh>
#include <G4VPhysicalVolume.hh>
#include <G4PVPlacement.hh>
#include <G4VisAttributes.hh>
#include <G4UserLimits.hh>

#include <G4Polyhedra.hh>

class CaptDriftRegionMessenger
    : public DSimBuilderMessenger {
private:
    CaptDriftRegionBuilder* fBuilder;
    G4UIcmdWithADoubleAndUnit* fApothemCMD;
    G4UIcmdWithADoubleAndUnit* fDriftLengthCMD;
    G4UIcmdWithADoubleAndUnit* fWirePlaneSpacingCMD;

public:
    CaptDriftRegionMessenger(CaptDriftRegionBuilder* c) 
        : DSimBuilderMessenger(c,"Control the drift region geometry."),
          fBuilder(c) {

        fApothemCMD
            = new G4UIcmdWithADoubleAndUnit(CommandName("length"),this);
        fApothemCMD->SetGuidance("Set the apothem of the drift region.");
        fApothemCMD->SetParameterName("apothem",false);
        fApothemCMD->SetUnitCategory("Length");

        fDriftLengthCMD = new G4UIcmdWithADoubleAndUnit(
            CommandName("driftLength"),this);
        fDriftLengthCMD->SetGuidance(
            "Set the drift length from cathode to wires.");
        fDriftLengthCMD->SetParameterName("drift",false);
        fDriftLengthCMD->SetUnitCategory("Length");

        fWirePlaneSpacingCMD = new G4UIcmdWithADoubleAndUnit(
            CommandName("wirePlaneSpacing"),this);
        fWirePlaneSpacingCMD->SetGuidance(
            "Set spacing between the wires.");
        fWirePlaneSpacingCMD->SetParameterName("space",false);
        fWirePlaneSpacingCMD->SetUnitCategory("Length");
    };

    virtual ~CaptDriftRegionMessenger() {
        delete fApothemCMD;
        delete fDriftLengthCMD;
    };

    void SetNewValue(G4UIcommand *cmd, G4String val) {
        if (cmd==fApothemCMD) {
            fBuilder->SetApothem(fApothemCMD->GetNewDoubleValue(val));
        }
        else if (cmd==fDriftLengthCMD) {
            fBuilder->SetDriftLength(fDriftLengthCMD->GetNewDoubleValue(val));
        }
        else if (cmd==fWirePlaneSpacingCMD) {
            fBuilder->SetWirePlaneSpacing(
                fWirePlaneSpacingCMD->GetNewDoubleValue(val));
        }
        else {
            DSimBuilderMessenger::SetNewValue(cmd,val);
        }
    };
};

void CaptDriftRegionBuilder::Init(void) {
    SetMessenger(new CaptDriftRegionMessenger(this));
    SetApothem(1000*mm);
    SetDriftLength(1200*mm);
    SetWirePlaneSpacing(3.1*mm);
    SetSensitiveDetector("drift","segment");
    SetMaximumHitLength(1*mm);
    SetMaximumHitSagitta(0.25*mm);

    AddBuilder(new CaptWirePlaneBuilder("XPlane",this));
    AddBuilder(new CaptWirePlaneBuilder("VPlane",this));
    AddBuilder(new CaptWirePlaneBuilder("UPlane",this));
}

CaptDriftRegionBuilder::~CaptDriftRegionBuilder() {};

double CaptDriftRegionBuilder::GetHeight() {
    CaptWirePlaneBuilder& wires = Get<CaptWirePlaneBuilder>("UPlane");
    return GetDriftLength() + 2*GetWirePlaneSpacing() + wires.GetHeight();
}

G4LogicalVolume *CaptDriftRegionBuilder::GetPiece(void) {

    double rInner[] = {0.0, 0.0};
    double rOuter[] = {GetApothem(), GetApothem()};
    double zPlane[] = {-GetHeight()/2, GetHeight()/2};

    G4LogicalVolume *logVolume
	= new G4LogicalVolume(new G4Polyhedra(GetName(),
                                              90*degree, 450*degree,
                                              6, 2,
                                              zPlane, rInner, rOuter),
                              FindMaterial("Argon_Liquid"),
                              GetName());

    if (GetSensitiveDetector()) {
        logVolume->SetSensitiveDetector(GetSensitiveDetector());
        logVolume->SetUserLimits(new G4UserLimits(1.0*mm));
    }

    // The wire planes are rotated so that the local Z axis points into the
    // drift volume, and the local X axis points along the original X axis
    // (this puts the local Y axis along the -Y global axis.
    G4RotationMatrix* xRotation = new G4RotationMatrix(); 
    xRotation->rotateX(180*degree);

    CaptWirePlaneBuilder& xPlane = Get<CaptWirePlaneBuilder>("XPlane");
    G4LogicalVolume *logX = xPlane.GetPiece();
    new G4PVPlacement(xRotation,                // rotation.
                      G4ThreeVector(0,0,
                                    (GetHeight()/2 
                                     - xPlane.GetHeight()/2)), 
                      logX,                     // logical volume
                      logX->GetName(),          // name
                      logVolume,                // mother  volume
                      false,                    // (not used)
                      0,                        // Copy number (zero)
                      Check());                 // Check overlaps.
    

    G4RotationMatrix* vRotation = new G4RotationMatrix(); 
    vRotation->rotateX(180*degree);
    vRotation->rotateZ(-60*degree);
                       
    CaptWirePlaneBuilder& vPlane = Get<CaptWirePlaneBuilder>("VPlane");
    G4LogicalVolume *logV = vPlane.GetPiece();
    new G4PVPlacement(vRotation,                // rotation.
                      G4ThreeVector(0,0,
                                    (GetHeight()/2 - xPlane.GetHeight()/2
                                     - GetWirePlaneSpacing())),
                      logV,                     // logical volume
                      logV->GetName(),          // name
                      logVolume,                // mother  volume
                      false,                    // (not used)
                      0,                        // Copy number (zero)
                      Check());                 // Check overlaps.


    G4RotationMatrix* uRotation = new G4RotationMatrix(); 
    uRotation->rotateX(180*degree);
    uRotation->rotateZ(60*degree);
    
    CaptWirePlaneBuilder& uPlane = Get<CaptWirePlaneBuilder>("UPlane");
    G4LogicalVolume *logU = uPlane.GetPiece();
    new G4PVPlacement(uRotation,                // rotation.
                      G4ThreeVector(0,0,
                                    (GetHeight()/2 - xPlane.GetHeight()/2
                                     - 2*GetWirePlaneSpacing())),
                      logU,                     // logical volume
                      logU->GetName(),          // name
                      logVolume,                // mother  volume
                      false,                    // (not used)
                      0,                        // Copy number (zero)
                      Check());                 // Check overlaps.
    
    if (GetVisible()) {
        logVolume->SetVisAttributes(GetColor(logVolume));
    } 
    else {
        logVolume->SetVisAttributes(G4VisAttributes::Invisible);
    }
    
    return logVolume;
}
