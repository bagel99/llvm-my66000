add_llvm_component_library(LLVMMy66000Desc
  My66000InstPrinter.cpp
  My66000MCTargetDesc.cpp
  My66000MCAsmInfo.cpp

  LINK_COMPONENTS
  Support
  My66000Info

  ADD_TO_COMPONENT
  My66000
  )
