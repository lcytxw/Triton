#include <iostream>
#include <sstream>
#include <stdexcept>

#include "JnbeIRBuilder.h"
#include "Registers.h"
#include "SMT2Lib.h"
#include "SymbolicElement.h"


JnbeIRBuilder::JnbeIRBuilder(uint64_t address, const std::string &disassembly):
  BaseIRBuilder(address, disassembly) {
}


void JnbeIRBuilder::imm(AnalysisProcessor &ap, Inst &inst) const {
  SymbolicElement   *se;
  std::stringstream expr, op1, op2;
  uint64_t          imm   = std::get<1>(this->operands[0]);
  uint64_t          symCF = ap.getRegSymbolicID(ID_CF);
  uint64_t          symZF = ap.getRegSymbolicID(ID_ZF);

  /* Create the SMT semantic */
  if (symCF != UNSET)
    op1 << "#" << std::dec << symCF;
  else
    op1 << smt2lib::bv(ap.getRegisterValue(ID_CF), 1);

  if (symZF != UNSET)
    op2 << "#" << std::dec << symZF;
  else
    op2 << smt2lib::bv(ap.getRegisterValue(ID_ZF), 1);

  /* 
   * Finale expr
   * JNBE: Jump if not below or equal (CF=0 and ZF=0).
   * SMT: (= (bvand (bvnot zf) (bvnot cf)) (_ bv1 1))
   */
  expr << smt2lib::ite(
            smt2lib::equal(
              smt2lib::bvand(
                smt2lib::bvnot(op1.str()),
                smt2lib::bvnot(op2.str())
              ),
              smt2lib::bvtrue()
            ),
            smt2lib::bv(imm, REG_SIZE_BIT),
            smt2lib::bv(this->nextAddress, REG_SIZE_BIT));

  /* Create the symbolic element */
  se = ap.createRegSE(expr, ID_RIP, "RIP");

  /* Add the constraint in the PathConstraints list */
  ap.addPathConstraint(se->getID());

  /* Add the symbolic element to the current inst */
  inst.addElement(se);
}


void JnbeIRBuilder::reg(AnalysisProcessor &ap, Inst &inst) const {
  OneOperandTemplate::stop(this->disas);
}


void JnbeIRBuilder::mem(AnalysisProcessor &ap, Inst &inst) const {
  OneOperandTemplate::stop(this->disas);
}


void JnbeIRBuilder::none(AnalysisProcessor &ap, Inst &inst) const {
  OneOperandTemplate::stop(this->disas);
}


Inst *JnbeIRBuilder::process(AnalysisProcessor &ap) const {
  this->checkSetup();

  Inst *inst = new Inst(ap.getThreadID(), this->address, this->disas);

  try {
    this->templateMethod(ap, *inst, this->operands, "JNBE");
    ap.incNumberOfExpressions(inst->numberOfElements()); /* Used for statistics */
  }
  catch (std::exception &e) {
    delete inst;
    throw;
  }

  return inst;
}

