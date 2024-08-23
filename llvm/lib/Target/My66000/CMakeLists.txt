add_llvm_component_group(My66000)

set(LLVM_TARGET_DEFINITIONS My66000.td)

tablegen(LLVM My66000GenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM My66000GenCallingConv.inc -gen-callingconv)
tablegen(LLVM My66000GenDAGISel.inc -gen-dag-isel)
tablegen(LLVM My66000GenInstrInfo.inc -gen-instr-info)
tablegen(LLVM My66000GenRegisterInfo.inc -gen-register-info)
tablegen(LLVM My66000GenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(My66000CommonTableGen)

add_llvm_target(My66000CodeGen
  My66000AsmPrinter.cpp
  My66000FrameLowering.cpp
  My66000InstrInfo.cpp
  My66000ISelDAGToDAG.cpp
  My66000ISelLowering.cpp
  My66000MachineFunctionInfo.cpp
  My66000MCInstLower.cpp
  My66000RegisterInfo.cpp
  My66000Subtarget.cpp
  My66000TargetMachine.cpp
  My66000TargetObjectFile.cpp
  My66000SelectionDAGInfo.cpp
  My66000ExpandPseudoInsts.cpp
  My66000PredicatePass.cpp
  My66000FixJumpTable.cpp
  My66000VVMPass1.cpp
  My66000VVMPass2.cpp

  LINK_COMPONENTS
  Analysis
  AsmPrinter
  CodeGen
  Core
  MC
  SelectionDAG
  Support
  Target
  TransformUtils
  My66000Desc
  My66000Info

  ADD_TO_COMPONENT
  My66000
  )

add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
