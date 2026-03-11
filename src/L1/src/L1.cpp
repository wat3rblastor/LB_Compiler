#include "L1.h"
#include "utils.h"

#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>

namespace L1 {

std::string Label::to_string() const {
  return name;
}

void Label::generate_code(std::ofstream& outputFile) const {
  std::string assemblyLabelName = "_" + name.substr(1);
  outputFile << assemblyLabelName;
}

std::string Number::to_string() const {
  return std::to_string(num);
}

void Number::generate_code(std::ofstream& outputFile) const {
  outputFile << num;
}

std::string Register::to_string() const {
  switch (id) {
    case rdi: return "rdi";
    case rsi: return "rsi";
    case rdx: return "rdx";
    case rcx: return "rcx";
    case r8:  return "r8";
    case r9:  return "r9";
    case rax: return "rax";
    case rbx: return "rbx";
    case rbp: return "rbp";
    case r10: return "r10";
    case r11: return "r11";
    case r12: return "r12";
    case r13: return "r13";
    case r14: return "r14";
    case r15: return "r15";
    case rsp: return "rsp";
  }
  throw std::runtime_error("Undefined RegisterID in register");
}

void Register::generate_code(std::ofstream& outputFile) const {
  switch (id) {
    case rdi: 
      outputFile << "%rdi";
      break;
    case rsi: 
      outputFile << "%rsi";
      break;
    case rdx:
      outputFile << "%rdx";
      break;
    case rcx: 
      outputFile << "%rcx";
      break;
    case r8:
      outputFile << "%r8";
      break;
    case r9:  
      outputFile << "%r9";
      break;
    case rax: 
      outputFile << "%rax";
      break;
    case rbx: 
      outputFile << "%rbx";
      break;
    case rbp: 
      outputFile << "%rbp";
      break;
    case r10: 
      outputFile << "%r10";
      break;
    case r11: 
      outputFile << "%r11";
      break;
    case r12: 
      outputFile << "%r12";
      break;
    case r13: 
      outputFile << "%r13";
      break;
    case r14: 
      outputFile << "%r14";
      break;
    case r15: 
      outputFile << "%r15";
      break;
    case rsp:
      outputFile << "%rsp";
  }
}

void Register::generate_code_8bit(std::ofstream& outputFile) const {
  switch (id) {
    case rdi: 
      outputFile << "%dil";
      break;
    case rsi: 
      outputFile << "%sil";
      break;
    case rdx:
      outputFile << "%dl";
      break;
    case rcx: 
      outputFile << "%cl";
      break;
    case r8:
      outputFile << "%r8b";
      break;
    case r9:  
      outputFile << "%r9b";
      break;
    case rax: 
      outputFile << "%al";
      break;
    case rbx: 
      outputFile << "%bl";
      break;
    case rbp: 
      outputFile << "%bpl";
      break;
    case r10: 
      outputFile << "%r10b";
      break;
    case r11: 
      outputFile << "%r11b";
      break;
    case r12: 
      outputFile << "%r12b";
      break;
    case r13: 
      outputFile << "%r13b";
      break;
    case r14: 
      outputFile << "%r14b";
      break;
    case r15: 
      outputFile << "%r15b";
      break;
  }
}

std::string MemoryAddress::to_string() const {
  std::ostringstream out;
  out << "mem " << reg->to_string() << " " << offset->to_string();
  return out.str();
}

void MemoryAddress::generate_code(std::ofstream& outputFile) const {
  offset->generate_code(outputFile);
  outputFile << "(";
  reg->generate_code(outputFile);
  outputFile << ")";
}

std::string ArithmeticOp::to_string() const {
  std::ostringstream out;
  std::string str_op;
  switch (op) {
    case ADD_EQUAL: 
      str_op = "+=";
      break;
    case SUB_EQUAL: 
      str_op = "-=";
      break;
    case MUL_EQUAL: 
      str_op = "*=";
      break;
    case AND_EQUAL: 
      str_op = "&=";
      break;
  }
  out << dst->to_string();
  out << " " << str_op << " ";
  out << src->to_string();
  return out.str();
}

void ArithmeticOp::generate_code(std::ofstream& outputFile) const {
  std::string assembly_op;
  switch (op) {
    case ADD_EQUAL: 
      assembly_op = "addq";
      break;
    case SUB_EQUAL: 
      assembly_op = "subq";
      break;
    case MUL_EQUAL: 
      assembly_op = "imulq";
      break;
    case AND_EQUAL:  
      assembly_op = "andq";
      break;
    default:
      throw std::runtime_error("Arithmetic::generate_code: wrong op");
  }
  outputFile << assembly_op << " ";
  if (dynamic_cast<Number*>(src.get())) {
    outputFile << "$";
  }
  src->generate_code(outputFile);
  outputFile << ", ";
  dst->generate_code(outputFile);
}

std::string ShiftOp::to_string() const {
  std::ostringstream out;
  std::string str_op;
  if (op == EnumShiftOps::LEFT_SHIFT) {
    str_op = "<<=";
  }
  else if (op == EnumShiftOps::RIGHT_SHIFT) {
    str_op = ">>=";
  }
  else {
    throw std::runtime_error("ShiftOp::to_string() invalid op");
  }
  out << dst->to_string();
  out << " " << str_op << " ";
  out << src->to_string();
  return out.str();
}

void ShiftOp::generate_code(std::ofstream& outputFile) const {
  if (op == EnumShiftOps::LEFT_SHIFT) {
    outputFile << "salq ";
  }
  else if (op == EnumShiftOps::RIGHT_SHIFT) {
    outputFile << "sarq ";
  }
  else {
    throw std::runtime_error("ShiftOp::to_string() invalid op");
  }

  if (auto reg = dynamic_cast<Register*>(src.get())) {
    reg->generate_code_8bit(outputFile);
  }
  else if (auto num = dynamic_cast<Number*>(src.get())) {
    outputFile << "$";
    num->generate_code(outputFile);
  }
  else {
    throw std::runtime_error("ShiftOp: src must be Register or Number");
  }

  outputFile << ", ";
  dst->generate_code(outputFile);
}

std::string CompareOp::to_string() const {
  std::string str_op;
  std::ostringstream out;
  switch (op) {
    case EnumCompareOps::LESS_THAN:
      str_op = "<";
      break;
    case EnumCompareOps::LESS_EQUAL:
      str_op = "<=";
      break;
    case EnumCompareOps::EQUAL:
      str_op = "=";
      break;
    default:
      throw std::runtime_error("CompareOp::to_string: Invalid op");
  }
  out << left->to_string();
  out << " " << str_op << " ";
  out << right->to_string();
  return out.str();
}

void CompareOp::generate_code(std::ofstream& outputFile) const {
  if (staticComputable) return;
  outputFile << "cmpq ";
  auto* l = dynamic_cast<Number*>(left.get());
  auto* r = dynamic_cast<Number*>(right.get());
  if (l) {
    outputFile << "$";
    left->generate_code(outputFile);
    outputFile << ", ";
    right->generate_code(outputFile);
  }
  else {
    if (r) outputFile << "$";
    right->generate_code(outputFile);
    outputFile << ", ";
    left->generate_code(outputFile);
  }
}

std::string AssignmentInstruction::to_string() const {
  std::ostringstream out;
  out << dst->to_string() << " <- " << src->to_string();
  return out.str();
}

void AssignmentInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "movq ";
  if (dynamic_cast<Number*>(src.get())
      || dynamic_cast<Label*>(src.get())) {
    outputFile << "$";
  }
  src->generate_code(outputFile);
  outputFile << ", ";
  dst->generate_code(outputFile);
}

std::string ArithmeticInstruction::to_string() const {
  return op->to_string();
}

void ArithmeticInstruction::generate_code(std::ofstream& outputFile) const {
  op->generate_code(outputFile);
}

std::string CompareInstruction::to_string() const {
  std::ostringstream out;
  out << dst->to_string();
  out << " <- ";
  out << op->to_string();
  return out.str();
}

void CompareInstruction::generate_code(std::ofstream& outputFile) const {
  if (op->staticComputable) {
    outputFile << "movq $" << op->staticValue << ", ";
    dst->generate_code(outputFile);
    return;
  }
  op->generate_code(outputFile);
  outputFile << std::endl << "\t";
  auto* l = dynamic_cast<Number*>(op->left.get());
  Register* reg = dynamic_cast<Register*>(dst.get());
  if (l) {
    switch (op->op) {
      case EnumCompareOps::LESS_THAN:
        outputFile << "setg ";
        break;
      case EnumCompareOps::LESS_EQUAL:
        outputFile << "setge ";
        break;
      case EnumCompareOps::EQUAL:
        outputFile << "sete ";
        break;
    }
  }
  else {
    switch (op->op) {
      case EnumCompareOps::LESS_THAN:
        outputFile << "setl ";
        break;
      case EnumCompareOps::LESS_EQUAL:
        outputFile << "setle ";
        break;
      case EnumCompareOps::EQUAL:
        outputFile << "sete ";
        break;
    }
  }
  reg->generate_code_8bit(outputFile);
  outputFile << std::endl << "\t";
  outputFile << "movzbq ";
  reg->generate_code_8bit(outputFile);
  outputFile << ", ";
  reg->generate_code(outputFile);
}

std::string ShiftInstruction::to_string() const {
  return op->to_string();
}

void ShiftInstruction::generate_code(std::ofstream& outputFile) const {
  return op->generate_code(outputFile);
}

std::string ConditionalJumpInstruction::to_string() const {
  std::ostringstream out;
  out << "cjump " << op->to_string();
  out << " " << label->to_string();
  return out.str();
}

void ConditionalJumpInstruction::generate_code(std::ofstream& outputFile) const {
  if (op->staticComputable) {
    if (op->staticValue == 1) {
      outputFile << "jmp ";
      label->generate_code(outputFile);
    }
    return;
  }
  op->generate_code(outputFile);
  outputFile << std::endl << "\t";
  auto* l = dynamic_cast<Number*>(op->left.get());
  if (l) {
    switch (op->op) {
      case EnumCompareOps::LESS_THAN:
        outputFile << "jg ";
        break;
      case EnumCompareOps::LESS_EQUAL:
        outputFile << "jge ";
        break;
      case EnumCompareOps::EQUAL:
        outputFile << "je ";
        break;
    }
  }
  else {
    switch (op->op) {
      case EnumCompareOps::LESS_THAN:
        outputFile << "jl ";
        break;
      case EnumCompareOps::LESS_EQUAL:
        outputFile << "jle ";
        break;
      case EnumCompareOps::EQUAL:
        outputFile << "je ";
        break;
    }
  }
  label->generate_code(outputFile);
}

std::string LabelInstruction::to_string() const {
  std::ostringstream out;
  out << label->to_string();
  return out.str();
}

void LabelInstruction::generate_code(std::ofstream& outputFile) const {
  label->generate_code(outputFile);
  outputFile << ":";
}

std::string JumpInstruction::to_string() const {
  std::ostringstream out;
  out << "goto " << label->to_string();
  return out.str();
}

void JumpInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "jmp ";
  label->generate_code(outputFile);
}

std::string ReturnInstruction::to_string() const {
  return "return";
}

void ReturnInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "retq";
}

std::string CallInstruction::to_string() const {
  std::ostringstream out;
  out << "call " << target->to_string() << " " << numArgs->to_string();
  return out.str();
}

void CallInstruction::generate_code(std::ofstream& outputFile) const {
  auto* num = dynamic_cast<Number*>(numArgs.get());
  if (!num) throw std::runtime_error("CallInstruction::generate_code: numArgs not Number");

  int64_t n = num->num;
  int64_t callFrameBytes = 8;               
  if (n > 6) callFrameBytes += 8 * (n - 6); 

  outputFile << "subq $" << callFrameBytes << ", %rsp\n\t";
  outputFile << "jmp ";
  if (dynamic_cast<Register*>(target.get())) outputFile << "*";
  target->generate_code(outputFile);
}

std::string CallPrintInstruction::to_string() const {
  return "call print 1";
}

void CallPrintInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "call print";
}

std::string CallAllocateInstruction::to_string() const {
  return "call allocate 2";
}

void CallAllocateInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "call allocate";
}

std::string CallInputInstruction::to_string() const {
  return "call input 0";
}

void CallInputInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "call input";
}

std::string CallTupleErrorInstruction::to_string() const {
  return "call tuple-error 3";
}

void CallTupleErrorInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "call tuple_error";
}

std::string CallTensorErrorInstruction::to_string() const {
  std::ostringstream out;
  out << "call tensor-error ";
  Number* fvalue = dynamic_cast<Number*>(f.get());
  out << fvalue->num;
  return out.str();
}

void CallTensorErrorInstruction::generate_code(std::ofstream& outputFile) const {
  Number* fvalue = dynamic_cast<Number*>(f.get());
  if (fvalue->num == 1) {
    outputFile << "call array_tensor_error_null";
  }
  else if (fvalue->num == 3) {
    outputFile << "call array_error";
  }
  else if (fvalue->num == 4) {
    outputFile << "call tensor_error";
  }
  else {
    throw std::runtime_error("CallTensorErrorInstruction::generate_code: invalid f");
  }
}

std::string IncrementInstruction::to_string() const {
  std::ostringstream out;
  out << reg->to_string();
  out << "++";
  return out.str();
}

void IncrementInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "inc ";
  reg->generate_code(outputFile);
}

std::string DecrementInstruction::to_string() const {
  std::ostringstream out;
  out << reg->to_string();
  out << "--";
  return out.str();
}

void DecrementInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "dec ";
  reg->generate_code(outputFile);
}

std::string LeaInstruction::to_string() const {
  std::ostringstream out;
  out << reg1->to_string() << " @ ";
  out << reg2->to_string() << " ";
  out << reg3->to_string() << " ";
  out << e->to_string();
  return out.str();
}

void LeaInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "lea (";
  reg2->generate_code(outputFile);
  outputFile << ", ";
  reg3->generate_code(outputFile);
  outputFile << ", ";
  e->generate_code(outputFile);
  outputFile << "), ";
  reg1->generate_code(outputFile);
}

std::string Function::to_string() const {
  std::ostringstream out;
  out << "\t(" << name.to_string() << "\n";
  out << "\t\t" << numArgs << " " << numLocal << "\n";
  for (const auto& instruction : instructions) {
    out << "\t\t" << instruction->to_string() << "\n";
  }
  out << "\t)\n";
  return out.str();
}

void Function::generate_code(std::ofstream& outputFile) const {
  name.generate_code(outputFile);
  outputFile << ":" << std::endl;
  int64_t frameBytes = numLocal * 8;
  // Prologue
  if (frameBytes != 0) {
    outputFile << "   subq $" << frameBytes << ", %rsp" << std::endl;
  }
  int64_t callFrameBytes = 8;
  if (numArgs > 6) callFrameBytes += 8 * (numArgs - 6);
  // Body
  size_t numInstructions = instructions.size();
  for (const auto& instruction : instructions) {
    // Epilogue
    if (dynamic_cast<ReturnInstruction*>(instruction.get())
        && (frameBytes + callFrameBytes - 8) != 0) {
      outputFile << "   addq $" << (frameBytes + callFrameBytes - 8) << ", %rsp" << std::endl;
    }
    // This just makes the code output cleaner
    if (!dynamic_cast<LabelInstruction*>(instruction.get())) {
      outputFile << "   ";
    }
    instruction->generate_code(outputFile);
    outputFile << std::endl;
  }
}

std::string Program::to_string() const {
  std::ostringstream out;
  out << "(" << entryPoint.to_string() << "\n";
  for (const auto& function : functions) {
    out << function->to_string() << "\n";
  }
  out << ")" << "\n";
  return out.str();
}

void Program::generate_code(std::ofstream& outputFile) const {
  std::string assemblyEntryPoint = "_" + entryPoint.name.substr(1);

  outputFile << "   .text" << std::endl;
  outputFile << "   .globl go" << std::endl;
  outputFile << std::endl;
  outputFile << "go:" << std::endl;
  outputFile << "   # save callee-saved registers" << std::endl;
  outputFile << "   pushq %rbx" << std::endl;
  outputFile << "   pushq %rbp" << std::endl;
  outputFile << "   pushq %r12" << std::endl;
  outputFile << "   pushq %r13" << std::endl;
  outputFile << "   pushq %r14" << std::endl;
  outputFile << "   pushq %r15" << std::endl;
  outputFile << std::endl;
  outputFile << "   call ";
  entryPoint.generate_code(outputFile);
  outputFile << std::endl << std::endl;
  outputFile << "   # restore callee-saved registers and return" << std::endl;
  outputFile << "   popq %r15" << std::endl;
  outputFile << "   popq %r14" << std::endl;
  outputFile << "   popq %r13" << std::endl;
  outputFile << "   popq %r12" << std::endl;
  outputFile << "   popq %rbp" << std::endl;
  outputFile << "   popq %rbx" << std::endl;
  outputFile << "   retq" << std::endl;
  outputFile << std::endl;
  outputFile << std::endl;

  for (const auto& function : functions) {
    function->generate_code(outputFile);
    outputFile << std::endl;
  }
}

}  // namespace L1