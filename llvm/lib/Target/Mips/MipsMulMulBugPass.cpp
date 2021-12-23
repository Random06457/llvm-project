#include "Mips.h"
#include "MipsInstrInfo.h"
#include "MipsSubtarget.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Target/TargetMachine.h"

#define DEBUG_TYPE "mips-r4300-mulmul-fix"

using namespace llvm;

static cl::opt<bool>
    EnableMulMulFix("mfix4300", cl::init(false),
                    cl::desc("Enable the VR4300 mulmul bug fix."), cl::Hidden);

class MipsMulMulBugFix : public MachineFunctionPass {
public:
  MipsMulMulBugFix() : MachineFunctionPass(ID) {}

  StringRef getPassName() const override { return "Mips mulmul bugfix"; }

  MachineFunctionProperties getRequiredProperties() const override {
    return MachineFunctionProperties().set(
        MachineFunctionProperties::Property::NoVRegs);
  }

  bool runOnMachineFunction(MachineFunction &MF) override;
  bool FixMulMulBB(MachineBasicBlock &MBB);

  static char ID;

private:
  static const MipsInstrInfo *MipsII;
  const MipsSubtarget *Subtarget;
};

char MipsMulMulBugFix::ID = 0;
const MipsInstrInfo *MipsMulMulBugFix::MipsII;

bool MipsMulMulBugFix::runOnMachineFunction(MachineFunction &MF) {

  if (!EnableMulMulFix)
    return false;

  Subtarget = &static_cast<const MipsSubtarget &>(MF.getSubtarget());
  MipsII = static_cast<const MipsInstrInfo *>(Subtarget->getInstrInfo());

  bool Modified = false;
  MachineFunction::iterator I = MF.begin(), E = MF.end();

  for (; I != E; ++I)
    Modified |= FixMulMulBB(*I);

  return Modified;
}

static bool isFirstMul(const MachineInstr *MI) {
  switch (MI->getOpcode()) {
  case Mips::MUL:
  case Mips::FMUL_S:
  case Mips::FMUL_D:
  case Mips::FMUL_D32:
  case Mips::FMUL_D64:
    return true;
  default:
    return false;
  }
}

static bool isSecondMulOrBranch(const MachineInstr *MI) {
  if (MI->isBranch() || MI->isIndirectBranch() || MI->isCall())
    return true;

  switch (MI->getOpcode()) {
  case Mips::MUL:
  case Mips::FMUL_S:
  case Mips::FMUL_D:
  case Mips::FMUL_D32:
  case Mips::FMUL_D64:
  case Mips::MULT:
  case Mips::MULTu:
  case Mips::DMULT:
  case Mips::DMULTu:
    return true;
  default:
    return false;
  }
}

bool MipsMulMulBugFix::FixMulMulBB(MachineBasicBlock &MBB) {
  bool Modified = false;
  MachineBasicBlock::instr_iterator MII = MBB.instr_begin(),
                                    E = MBB.instr_end();
  MachineBasicBlock::instr_iterator NextMII;

  // Iterate through the instructions in the basic block
  for (; MII != E; MII = NextMII) {

    NextMII = std::next(MII);
    MachineInstr *MI = &*MII;

    // Trigger when the current instruction is a mul and the next instruction
    // is either a mul or a branch in case the branch target start with a mul
    if (NextMII != E && isFirstMul(MI) && isSecondMulOrBranch(&*NextMII)) {
      LLVM_DEBUG(dbgs() << "Found mulmul!");

      MachineBasicBlock &MBB = *MI->getParent();
      const MCInstrDesc &NewMCID = MipsII->get(Mips::NOP);
      BuildMI(MBB, NextMII, DebugLoc(), NewMCID);
      Modified = true;
    }
  }

  return Modified;
}

/// createMipsMulMulBugPass - Returns a pass that fixes the mulmul bug
FunctionPass *llvm::createMipsMulMulBugPass() { return new MipsMulMulBugFix(); }
