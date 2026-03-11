#include "instructions.h"

namespace L2 {
std::string AssignmentInstruction::to_string() const {
  std::ostringstream out;
  out << dst->to_string() << " <- " << src->to_string();
  return out.str();
}

void AssignmentInstruction::generate_code(std::ofstream& outputFile) const {
  dst->generate_code(outputFile);
  outputFile << " <- ";
  src->generate_code(outputFile);
}

std::unique_ptr<ASTNode> AssignmentInstruction::clone() const {
  return std::make_unique<AssignmentInstruction>(dst->clone(), src->clone());
}

GenKill AssignmentInstruction::generate_GEN_KILL() {
  GenKill gk;
  if (Register* reg = dynamic_cast<Register*>(dst.get())) {
    gk.kill_locations.insert(reg->id);
  } else if (Var* var = dynamic_cast<Var*>(dst.get())) {
    gk.kill_locations.insert(var->name);
  } else if (MemoryAddress* memAddr = dynamic_cast<MemoryAddress*>(dst.get())) {
    if (Register* reg = dynamic_cast<Register*>(memAddr->reg.get())) {
      gk.gen_locations.insert(reg->id);
    } else if (Var* var = dynamic_cast<Var*>(memAddr->reg.get())) {
      gk.gen_locations.insert(var->name);
    }
  }

  if (Register* reg = dynamic_cast<Register*>(src.get())) {
    gk.gen_locations.insert(reg->id);
  } else if (Var* var = dynamic_cast<Var*>(src.get())) {
    gk.gen_locations.insert(var->name);
  } else if (MemoryAddress* memAddr = dynamic_cast<MemoryAddress*>(src.get())) {
    if (Register* reg = dynamic_cast<Register*>(memAddr->reg.get())) {
      gk.gen_locations.insert(reg->id);
    } else if (Var* var = dynamic_cast<Var*>(memAddr->reg.get())) {
      gk.gen_locations.insert(var->name);
    }
  }
  return gk;
}

std::string ArithmeticInstruction::to_string() const { return op->to_string(); }

void ArithmeticInstruction::generate_code(std::ofstream& outputFile) const {
  op->generate_code(outputFile);
}

std::unique_ptr<ASTNode> ArithmeticInstruction::clone() const {
  std::unique_ptr<ASTNode> astOp = op->clone();
  std::unique_ptr<ArithmeticOp> op(static_cast<ArithmeticOp*>(astOp.release()));
  return std::make_unique<ArithmeticInstruction>(std::move(op));
}

GenKill ArithmeticInstruction::generate_GEN_KILL() {
  GenKill gk;

  ASTNode* dst = op->dst.get();
  ASTNode* src = op->src.get();

  if (Register* reg = dynamic_cast<Register*>(dst)) {
    gk.gen_locations.insert(reg->id);
    gk.kill_locations.insert(reg->id);
  } else if (Var* var = dynamic_cast<Var*>(dst)) {
    gk.gen_locations.insert(var->name);
    gk.kill_locations.insert(var->name);
  } else if (MemoryAddress* memAddr = dynamic_cast<MemoryAddress*>(dst)) {
    if (Register* reg = dynamic_cast<Register*>(memAddr->reg.get())) {
      gk.gen_locations.insert(reg->id);
    } else if (Var* var = dynamic_cast<Var*>(memAddr->reg.get())) {
      gk.gen_locations.insert(var->name);
    }
  }
  if (Register* reg = dynamic_cast<Register*>(src)) {
    gk.gen_locations.insert(reg->id);
  } else if (Var* var = dynamic_cast<Var*>(src)) {
    gk.gen_locations.insert(var->name);
  } else if (MemoryAddress* memAddr = dynamic_cast<MemoryAddress*>(src)) {
    if (Register* reg = dynamic_cast<Register*>(memAddr->reg.get())) {
      gk.gen_locations.insert(reg->id);
    } else if (Var* var = dynamic_cast<Var*>(memAddr->reg.get())) {
      gk.gen_locations.insert(var->name);
    }
  }
  return gk;
}

std::string CompareInstruction::to_string() const {
  std::ostringstream out;
  out << dst->to_string();
  out << " <- ";
  out << op->to_string();
  return out.str();
}

void CompareInstruction::generate_code(std::ofstream& outputFile) const {
  dst->generate_code(outputFile);
  outputFile << " <- ";
  op->generate_code(outputFile);
}

std::unique_ptr<ASTNode> CompareInstruction::clone() const {
  std::unique_ptr<ASTNode> astOp = op->clone();
  std::unique_ptr<CompareOp> op(static_cast<CompareOp*>(astOp.release()));
  std::unique_ptr<ASTNode> dstClone = dst->clone();
  return std::make_unique<CompareInstruction>(std::move(dstClone), std::move(op));
}

GenKill CompareInstruction::generate_GEN_KILL() {
  GenKill gk;

  if (Register* reg = dynamic_cast<Register*>(dst.get())) {
    gk.kill_locations.insert(reg->id);
  } else if (Var* var = dynamic_cast<Var*>(dst.get())) {
    gk.kill_locations.insert(var->name);
  } else if (MemoryAddress* memAddr = dynamic_cast<MemoryAddress*>(dst.get())) {
    if (Register* reg = dynamic_cast<Register*>(memAddr->reg.get())) {
      gk.gen_locations.insert(reg->id);
    } else if (Var* var = dynamic_cast<Var*>(memAddr->reg.get())) {
      gk.gen_locations.insert(var->name);
    }
  }

  ASTNode* left = op->left.get();
  ASTNode* right = op->right.get();

  if (Register* reg = dynamic_cast<Register*>(left)) {
    gk.gen_locations.insert(reg->id);
  } else if (Var* var = dynamic_cast<Var*>(left)) {
    gk.gen_locations.insert(var->name);
  } else if (MemoryAddress* memAddr = dynamic_cast<MemoryAddress*>(left)) {
    if (Register* reg = dynamic_cast<Register*>(memAddr->reg.get())) {
      gk.gen_locations.insert(reg->id);
    } else if (Var* var = dynamic_cast<Var*>(memAddr->reg.get())) {
      gk.gen_locations.insert(var->name);
    }
  }
  if (Register* reg = dynamic_cast<Register*>(right)) {
    gk.gen_locations.insert(reg->id);
  } else if (Var* var = dynamic_cast<Var*>(right)) {
    gk.gen_locations.insert(var->name);
  } else if (MemoryAddress* memAddr = dynamic_cast<MemoryAddress*>(right)) {
    if (Register* reg = dynamic_cast<Register*>(memAddr->reg.get())) {
      gk.gen_locations.insert(reg->id);
    } else if (Var* var = dynamic_cast<Var*>(memAddr->reg.get())) {
      gk.gen_locations.insert(var->name);
    }
  }
  return gk;
}

std::string ShiftInstruction::to_string() const { return op->to_string(); }

void ShiftInstruction::generate_code(std::ofstream& outputFile) const {
  op->generate_code(outputFile);
}

std::unique_ptr<ASTNode> ShiftInstruction::clone() const {
  std::unique_ptr<ASTNode> astOp = op->clone();
  std::unique_ptr<ShiftOp> op(static_cast<ShiftOp*>(astOp.release()));
  return std::make_unique<ShiftInstruction>(std::move(op));
}

GenKill ShiftInstruction::generate_GEN_KILL() {
  GenKill gk;

  ASTNode* dst = op->dst.get();
  ASTNode* src = op->src.get();

  if (Register* reg = dynamic_cast<Register*>(dst)) {
    gk.gen_locations.insert(reg->id);
    gk.kill_locations.insert(reg->id);
  } else if (Var* var = dynamic_cast<Var*>(dst)) {
    gk.gen_locations.insert(var->name);
    gk.kill_locations.insert(var->name);
  } else if (MemoryAddress* memAddr = dynamic_cast<MemoryAddress*>(dst)) {
    if (Register* reg = dynamic_cast<Register*>(memAddr->reg.get())) {
      gk.gen_locations.insert(reg->id);
    } else if (Var* var = dynamic_cast<Var*>(memAddr->reg.get())) {
      gk.gen_locations.insert(var->name);
    }
  }

  if (Register* reg = dynamic_cast<Register*>(src)) {
    gk.gen_locations.insert(reg->id);
  } else if (Var* var = dynamic_cast<Var*>(src)) {
    gk.gen_locations.insert(var->name);
  } else if (MemoryAddress* memAddr = dynamic_cast<MemoryAddress*>(src)) {
    if (Register* reg = dynamic_cast<Register*>(memAddr->reg.get())) {
      gk.gen_locations.insert(reg->id);
    } else if (Var* var = dynamic_cast<Var*>(memAddr->reg.get())) {
      gk.gen_locations.insert(var->name);
    }
  }
  return gk;
}

std::string ConditionalJumpInstruction::to_string() const {
  std::ostringstream out;
  out << "cjump " << op->to_string();
  out << " " << label->to_string();
  return out.str();
}

void ConditionalJumpInstruction::generate_code(
    std::ofstream& outputFile) const {
  outputFile << "cjump ";
  op->generate_code(outputFile);
  outputFile << " ";
  label->generate_code(outputFile);
}

std::unique_ptr<ASTNode> ConditionalJumpInstruction::clone() const {
  std::unique_ptr<ASTNode> astOp = op->clone();
  std::unique_ptr<CompareOp> op(static_cast<CompareOp*>(astOp.release()));
  std::unique_ptr<ASTNode> labelClone = label->clone();
  return std::make_unique<ConditionalJumpInstruction>(std::move(op), std::move(labelClone));
}

GenKill ConditionalJumpInstruction::generate_GEN_KILL() {
  GenKill gk;

  ASTNode* left = op->left.get();
  ASTNode* right = op->right.get();

  if (Register* reg = dynamic_cast<Register*>(left)) {
    gk.gen_locations.insert(reg->id);
  } else if (Var* var = dynamic_cast<Var*>(left)) {
    gk.gen_locations.insert(var->name);
  } else if (MemoryAddress* memAddr = dynamic_cast<MemoryAddress*>(left)) {
    if (Register* reg = dynamic_cast<Register*>(memAddr->reg.get())) {
      gk.gen_locations.insert(reg->id);
    } else if (Var* var = dynamic_cast<Var*>(memAddr->reg.get())) {
      gk.gen_locations.insert(var->name);
    }
  }
  if (Register* reg = dynamic_cast<Register*>(right)) {
    gk.gen_locations.insert(reg->id);
  } else if (Var* var = dynamic_cast<Var*>(right)) {
    gk.gen_locations.insert(var->name);
  } else if (MemoryAddress* memAddr = dynamic_cast<MemoryAddress*>(right)) {
    if (Register* reg = dynamic_cast<Register*>(memAddr->reg.get())) {
      gk.gen_locations.insert(reg->id);
    } else if (Var* var = dynamic_cast<Var*>(memAddr->reg.get())) {
      gk.gen_locations.insert(var->name);
    }
  }
  return gk;
}

std::string LabelInstruction::to_string() const {
  std::ostringstream out;
  out << label->to_string();
  return out.str();
}

void LabelInstruction::generate_code(std::ofstream& outputFile) const {
  label->generate_code(outputFile);
}

std::unique_ptr<ASTNode> LabelInstruction::clone() const {
  return std::make_unique<LabelInstruction>(label->clone());
}

GenKill LabelInstruction::generate_GEN_KILL() { return GenKill(); }

std::string JumpInstruction::to_string() const {
  std::ostringstream out;
  out << "goto " << label->to_string();
  return out.str();
}

void JumpInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "goto ";
  label->generate_code(outputFile);
}

std::unique_ptr<ASTNode> JumpInstruction::clone() const {
  return std::make_unique<JumpInstruction>(label->clone());
}

GenKill JumpInstruction::generate_GEN_KILL() { return GenKill(); }

std::string ReturnInstruction::to_string() const { return "return"; }

void ReturnInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "return";
}

std::unique_ptr<ASTNode> ReturnInstruction::clone() const {
  return std::make_unique<ReturnInstruction>();
}

GenKill ReturnInstruction::generate_GEN_KILL() {
  GenKill gk;
  gk.gen_locations.insert(RegisterID::rax);
  gk.gen_locations.insert(RegisterID::r12);
  gk.gen_locations.insert(RegisterID::r13);
  gk.gen_locations.insert(RegisterID::r14);
  gk.gen_locations.insert(RegisterID::r15);
  gk.gen_locations.insert(RegisterID::rbp);
  gk.gen_locations.insert(RegisterID::rbx);
  return gk;
}

std::string CallInstruction::to_string() const {
  std::ostringstream out;
  out << "call " << target->to_string() << " " << numArgs->to_string();
  return out.str();
}

void CallInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "call ";
  target->generate_code(outputFile);
  outputFile << " ";
  numArgs->generate_code(outputFile);
}

std::unique_ptr<ASTNode> CallInstruction::clone() const {
  return std::make_unique<CallInstruction>(target->clone(), numArgs->clone());
}

GenKill CallInstruction::generate_GEN_KILL() {
  GenKill gk;

  if (auto* reg = dynamic_cast<Register*>(target.get()))
    gk.gen_locations.insert(reg->id);
  else if (auto* var = dynamic_cast<Var*>(target.get()))
    gk.gen_locations.insert(var->name);

  auto* num = dynamic_cast<Number*>(numArgs.get());
  int64_t n = num ? num->num : 0;
  if (n >= 1) gk.gen_locations.insert(RegisterID::rdi);
  if (n >= 2) gk.gen_locations.insert(RegisterID::rsi);
  if (n >= 3) gk.gen_locations.insert(RegisterID::rdx);
  if (n >= 4) gk.gen_locations.insert(RegisterID::rcx);
  if (n >= 5) gk.gen_locations.insert(RegisterID::r8);
  if (n >= 6) gk.gen_locations.insert(RegisterID::r9);

  for (RegisterID id : {RegisterID::rax, RegisterID::rcx, RegisterID::rdx,
                        RegisterID::rsi, RegisterID::rdi, RegisterID::r8,
                        RegisterID::r9, RegisterID::r10, RegisterID::r11}) {
    gk.kill_locations.insert(id);
  }

  return gk;
}

std::string CallPrintInstruction::to_string() const { return "call print 1"; }

void CallPrintInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "call print 1";
}

std::unique_ptr<ASTNode> CallPrintInstruction::clone() const {
  return std::make_unique<CallPrintInstruction>();
}

GenKill CallPrintInstruction::generate_GEN_KILL() {
  GenKill gk;
  gk.gen_locations.insert(RegisterID::rdi);
  for (RegisterID id : {RegisterID::rax, RegisterID::rcx, RegisterID::rdx,
                        RegisterID::rsi, RegisterID::rdi, RegisterID::r8,
                        RegisterID::r9, RegisterID::r10, RegisterID::r11}) {
    gk.kill_locations.insert(id);
  }
  return gk;
}

std::string CallAllocateInstruction::to_string() const {
  return "call allocate 2";
}

void CallAllocateInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "call allocate 2";
}

std::unique_ptr<ASTNode> CallAllocateInstruction::clone() const {
  return std::make_unique<CallAllocateInstruction>();
}

GenKill CallAllocateInstruction::generate_GEN_KILL() {
  GenKill gk;
  gk.gen_locations.insert(RegisterID::rdi);
  gk.gen_locations.insert(RegisterID::rsi);
  for (RegisterID id : {RegisterID::rax, RegisterID::rcx, RegisterID::rdx,
                        RegisterID::rsi, RegisterID::rdi, RegisterID::r8,
                        RegisterID::r9, RegisterID::r10, RegisterID::r11}) {
    gk.kill_locations.insert(id);
  }
  return gk;
}

std::string CallInputInstruction::to_string() const { return "call input 0"; }

void CallInputInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "call input 0";
}

std::unique_ptr<ASTNode> CallInputInstruction::clone() const {
  return std::make_unique<CallInputInstruction>();
}

GenKill CallInputInstruction::generate_GEN_KILL() {
  GenKill gk;
  for (RegisterID id : {RegisterID::rax, RegisterID::rcx, RegisterID::rdx,
                        RegisterID::rsi, RegisterID::rdi, RegisterID::r8,
                        RegisterID::r9, RegisterID::r10, RegisterID::r11}) {
    gk.kill_locations.insert(id);
  }
  return gk;
}

std::string CallTupleErrorInstruction::to_string() const {
  return "call tuple-error 3";
}

void CallTupleErrorInstruction::generate_code(std::ofstream& outputFile) const {
  outputFile << "call tuple-error 3";
}

std::unique_ptr<ASTNode> CallTupleErrorInstruction::clone() const {
  return std::make_unique<CallTupleErrorInstruction>();
}

GenKill CallTupleErrorInstruction::generate_GEN_KILL() {
  GenKill gk;
  gk.gen_locations.insert(RegisterID::rdi);
  gk.gen_locations.insert(RegisterID::rsi);
  gk.gen_locations.insert(RegisterID::rdx);
  for (RegisterID id : {RegisterID::rax, RegisterID::rcx, RegisterID::rdx,
                        RegisterID::rsi, RegisterID::rdi, RegisterID::r8,
                        RegisterID::r9, RegisterID::r10, RegisterID::r11}) {
    gk.kill_locations.insert(id);
  }
  return gk;
}

std::string CallTensorErrorInstruction::to_string() const {
  std::ostringstream out;
  out << "call tensor-error ";
  Number* fvalue = dynamic_cast<Number*>(f.get());
  out << fvalue->num;
  return out.str();
}

void CallTensorErrorInstruction::generate_code(
    std::ofstream& outputFile) const {
  outputFile << "call tensor-error ";
  f->generate_code(outputFile);
}

std::unique_ptr<ASTNode> CallTensorErrorInstruction::clone() const {
  return std::make_unique<CallTensorErrorInstruction>(f->clone());
}

GenKill CallTensorErrorInstruction::generate_GEN_KILL() {
  GenKill gk;
  auto* num = dynamic_cast<Number*>(f.get());
  int64_t n = num ? num->num : 0;

  if (n >= 1) gk.gen_locations.insert(RegisterID::rdi);
  if (n >= 2) gk.gen_locations.insert(RegisterID::rsi);
  if (n >= 3) gk.gen_locations.insert(RegisterID::rdx);
  if (n >= 4) gk.gen_locations.insert(RegisterID::rcx);
  if (n >= 5) gk.gen_locations.insert(RegisterID::r8);
  if (n >= 6) gk.gen_locations.insert(RegisterID::r9);
  for (RegisterID id : {RegisterID::rax, RegisterID::rcx, RegisterID::rdx,
                        RegisterID::rsi, RegisterID::rdi, RegisterID::r8,
                        RegisterID::r9, RegisterID::r10, RegisterID::r11}) {
    gk.kill_locations.insert(id);
  }
  return gk;
}

std::string IncrementInstruction::to_string() const {
  std::ostringstream out;
  out << reg->to_string();
  out << "++";
  return out.str();
}

void IncrementInstruction::generate_code(std::ofstream& outputFile) const {
  reg->generate_code(outputFile);
  outputFile << "++";
}

std::unique_ptr<ASTNode> IncrementInstruction::clone() const {
  return std::make_unique<IncrementInstruction>(reg->clone());
}

GenKill IncrementInstruction::generate_GEN_KILL() {
  GenKill gk;
  if (Register* r = dynamic_cast<Register*>(reg.get())) {
    gk.gen_locations.insert(r->id);
    gk.kill_locations.insert(r->id);
  } else if (Var* v = dynamic_cast<Var*>(reg.get())) {
    gk.gen_locations.insert(v->name);
    gk.kill_locations.insert(v->name);
  }
  return gk;
}

std::string DecrementInstruction::to_string() const {
  std::ostringstream out;
  out << reg->to_string();
  out << "--";
  return out.str();
}

void DecrementInstruction::generate_code(std::ofstream& outputFile) const {
  reg->generate_code(outputFile);
  outputFile << "--";
}

std::unique_ptr<ASTNode> DecrementInstruction::clone() const {
  return std::make_unique<DecrementInstruction>(reg->clone());
}

GenKill DecrementInstruction::generate_GEN_KILL() {
  GenKill gk;
  if (Register* r = dynamic_cast<Register*>(reg.get())) {
    gk.gen_locations.insert(r->id);
    gk.kill_locations.insert(r->id);
  } else if (Var* v = dynamic_cast<Var*>(reg.get())) {
    gk.gen_locations.insert(v->name);
    gk.kill_locations.insert(v->name);
  }
  return gk;
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
  reg1->generate_code(outputFile);
  outputFile << " @ ";
  reg2->generate_code(outputFile);
  outputFile << " ";
  reg3->generate_code(outputFile);
  outputFile << " ";
  e->generate_code(outputFile);
 
}

std::unique_ptr<ASTNode> LeaInstruction::clone() const {
  return std::make_unique<LeaInstruction>(reg1->clone(), reg2->clone(),
                                          reg3->clone(), e->clone());
}

GenKill LeaInstruction::generate_GEN_KILL() {
  GenKill gk;
  if (Register* reg = dynamic_cast<Register*>(reg2.get())) {
    gk.gen_locations.insert(reg->id);
  } else if (Var* var = dynamic_cast<Var*>(reg2.get())) {
    gk.gen_locations.insert(var->name);
  } else {
    throw std::runtime_error("LeaInstruction reg2 neither reg nor var");
  }

  if (Register* reg = dynamic_cast<Register*>(reg3.get())) {
    gk.gen_locations.insert(reg->id);
  } else if (Var* var = dynamic_cast<Var*>(reg3.get())) {
    gk.gen_locations.insert(var->name);
  } else {
    throw std::runtime_error("LeaInstruction reg3 neither reg nor var");
  }

  if (Register* reg = dynamic_cast<Register*>(reg1.get())) {
    gk.kill_locations.insert(reg->id);
  } else if (Var* var = dynamic_cast<Var*>(reg1.get())) {
    gk.kill_locations.insert(var->name);
  } else {
    throw std::runtime_error("LeaInstruction reg1 neither reg nor var");
  }
  return gk;
}

}  // namespace L2