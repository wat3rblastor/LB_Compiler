#include "instruction_selection.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stack>

#include "utils.h"

namespace L3 {

std::unique_ptr<TreeNode> TreeNodeFactory::make_var(Var var) {
  std::unique_ptr<TreeNode> varNode = std::make_unique<TreeNode>();
  varNode->type = NodeType::VAR;
  varNode->name = var.name;
  return varNode;
}

std::unique_ptr<TreeNode> TreeNodeFactory::make_number(Number num) {
  std::unique_ptr<TreeNode> numNode = std::make_unique<TreeNode>();
  numNode->type = NodeType::NUMBER;
  numNode->value = num.value;
  return numNode;
}

std::unique_ptr<TreeNode> TreeNodeFactory::make_label(Label label) {
  std::unique_ptr<TreeNode> labelNode = std::make_unique<TreeNode>();
  labelNode->type = NodeType::LABEL;
  labelNode->name = label.name;
  return labelNode;
}

std::unique_ptr<TreeNode> TreeNodeFactory::make_arithmetic_op(
    ArithmeticOp arithmeticOp) {
  EnumArithmeticOp op = arithmeticOp.get_op();
  std::unique_ptr<TreeNode> opNode = std::make_unique<TreeNode>();
  switch (op) {
    case EnumArithmeticOp::ADD:
      opNode->type = NodeType::ADD;
      break;
    case EnumArithmeticOp::SUB:
      opNode->type = NodeType::SUB;
      break;
    case EnumArithmeticOp::MUL:
      opNode->type = NodeType::MUL;
      break;
    case EnumArithmeticOp::AND:
      opNode->type = NodeType::AND;
      break;
    case EnumArithmeticOp::LSHIFT:
      opNode->type = NodeType::LSHIFT;
      break;
    case EnumArithmeticOp::RSHIFT:
      opNode->type = NodeType::RSHIFT;
      break;
    default:
      throw std::runtime_error(
          "TreeNodeFactory::make_arithmetic_op: Invalid EnumArithmeticOp");
  }

  Operand left = arithmeticOp.get_left();
  std::unique_ptr<TreeNode> leftNode = std::make_unique<TreeNode>();
  std::visit(
      [&leftNode](auto&& left) {
        using T = std::decay_t<decltype(left)>;
        if constexpr (std::is_same_v<T, Var>) {
          leftNode->name = left.name;
          leftNode->type = NodeType::VAR;
        } else if constexpr (std::is_same_v<T, Number>) {
          leftNode->value = left.value;
          leftNode->type = NodeType::NUMBER;
        } else {
          throw std::runtime_error(
              "TreeNodeFactory::make_arithmetic_op: Invalid Operand");
        }
      },
      left);

  Operand right = arithmeticOp.get_right();
  std::unique_ptr<TreeNode> rightNode = std::make_unique<TreeNode>();
  std::visit(
      [&rightNode](auto&& right) {
        using T = std::decay_t<decltype(right)>;
        if constexpr (std::is_same_v<T, Var>) {
          rightNode->name = right.name;
          rightNode->type = NodeType::VAR;
        } else if constexpr (std::is_same_v<T, Number>) {
          rightNode->value = right.value;
          rightNode->type = NodeType::NUMBER;
        } else {
          throw std::runtime_error(
              "TreeNodeFactory::make_arithmetic_op: Invalid Operand");
        }
      },
      right);

  opNode->children.push_back(std::move(leftNode));
  opNode->children.push_back(std::move(rightNode));
  return opNode;
}

std::unique_ptr<TreeNode> TreeNodeFactory::make_compare_op(
    CompareOp compareOp) {
  EnumCompareOp op = compareOp.get_op();
  std::unique_ptr<TreeNode> opNode = std::make_unique<TreeNode>();
  switch (op) {
    case EnumCompareOp::LT:
      opNode->type = NodeType::LT;
      break;
    case EnumCompareOp::LE:
      opNode->type = NodeType::LE;
      break;
    case EnumCompareOp::EQ:
      opNode->type = NodeType::EQ;
      break;
    case EnumCompareOp::GE:
      opNode->type = NodeType::GE;
      break;
    case EnumCompareOp::GT:
      opNode->type = NodeType::GT;
      break;
    default:
      throw std::runtime_error(
          "TreeNodeFactory::make_compare_op: Invalid EnumCompareOp");
  }

  Operand left = compareOp.get_left();
  std::unique_ptr<TreeNode> leftNode = std::make_unique<TreeNode>();
  std::visit(
      [&leftNode](auto&& left) {
        using T = std::decay_t<decltype(left)>;
        if constexpr (std::is_same_v<T, Var>) {
          leftNode->name = left.name;
          leftNode->type = NodeType::VAR;
        } else if constexpr (std::is_same_v<T, Number>) {
          leftNode->value = left.value;
          leftNode->type = NodeType::NUMBER;
        } else {
          throw std::runtime_error(
              "TreeNodeFactory::make_compare_op: Invalid Operand");
        }
      },
      left);

  Operand right = compareOp.get_right();
  std::unique_ptr<TreeNode> rightNode = std::make_unique<TreeNode>();
  std::visit(
      [&rightNode](auto&& right) {
        using T = std::decay_t<decltype(right)>;
        if constexpr (std::is_same_v<T, Var>) {
          rightNode->name = right.name;
          rightNode->type = NodeType::VAR;
        } else if constexpr (std::is_same_v<T, Number>) {
          rightNode->value = right.value;
          rightNode->type = NodeType::NUMBER;
        } else {
          throw std::runtime_error(
              "TreeNodeFactory::make_compare_op: Invalid Operand");
        }
      },
      right);

  opNode->children.push_back(std::move(leftNode));
  opNode->children.push_back(std::move(rightNode));
  return opNode;
}

std::unique_ptr<TreeNode> TreeNodeFactory::make_arrow() {
  std::unique_ptr<TreeNode> arrowNode = std::make_unique<TreeNode>();
  arrowNode->type = NodeType::ARROW;
  return arrowNode;
}

std::unique_ptr<TreeNode> TreeNodeFactory::make_load() {
  std::unique_ptr<TreeNode> loadNode = std::make_unique<TreeNode>();
  loadNode->type = NodeType::LOAD;
  return loadNode;
}

std::unique_ptr<TreeNode> TreeNodeFactory::make_store() {
  std::unique_ptr<TreeNode> storeNode = std::make_unique<TreeNode>();
  storeNode->type = NodeType::STORE;
  return storeNode;
}

std::unique_ptr<TreeNode> TreeNodeFactory::make_return() {
  std::unique_ptr<TreeNode> returnNode = std::make_unique<TreeNode>();
  returnNode->type = NodeType::RETURN;
  return returnNode;
}

std::unique_ptr<TreeNode> TreeNodeFactory::make_break() {
  std::unique_ptr<TreeNode> breakNode = std::make_unique<TreeNode>();
  breakNode->type = NodeType::BR;
  return breakNode;
}

void InstructionSelector::load_tiles() {
  TreeTile assignVarNumLabel;
  assignVarNumLabel.numInstructions = 1;
  assignVarNumLabel.numNodes = 3;
  assignVarNumLabel.match = [](TreeNode* node) {
    if (!node) return false;
    if (node->type != NodeType::VAR) return false;
    if (node->children.size() != 1) return false;
    node = node->children[0].get();
    if (node->type != NodeType::ARROW) return false;
    if (node->children.size() != 1) return false;
    node = node->children[0].get();
    if (!(node->type == NodeType::VAR || node->type == NodeType::NUMBER ||
          node->type == NodeType::LABEL)) {
      return false;
    }
    return true;
  };
  assignVarNumLabel.generate_L2_code = [](TreeNode* node) {
    std::ostringstream out;
    out << node->name;
    node = node->children[0].get();
    out << " <- ";
    node = node->children[0].get();
    if (node->type == NodeType::VAR || node->type == NodeType::LABEL) {
      out << node->name;
    } else {
      out << node->value;
    }
    return std::vector<std::string>{out.str()};
  };
  assignVarNumLabel.find_children = [](TreeNode* node) {
    std::vector<TreeNode*> children;
    node = node->children[0].get();
    node = node->children[0].get();
    if (!node->children.empty()) {
      children.push_back(node);
    }
    return children;
  };

  TreeTile assignArithOpTile;
  assignArithOpTile.numNodes = 5;
  // Technically can be 1, and I will fix tis if I have the time
  assignArithOpTile.numInstructions = 2;
  assignArithOpTile.match = [](TreeNode* node) {
    if (!node) return false;
    if (node->type != NodeType::VAR) return false;
    if (node->children.size() != 1) return false;
    node = node->children[0].get();
    if (node->type != NodeType::ARROW) return false;
    if (node->children.size() != 1) return false;
    node = node->children[0].get();
    if (!(node->type == NodeType::ADD || node->type == NodeType::SUB ||
          node->type == NodeType::MUL || node->type == NodeType::AND ||
          node->type == NodeType::LSHIFT || node->type == NodeType::RSHIFT)) {
      return false;
    }
    if (node->children.size() != 2) return false;
    TreeNode* left = node->children[0].get();
    TreeNode* right = node->children[1].get();
    if (!(left->type == NodeType::VAR || left->type == NodeType::NUMBER))
      return false;
    if (!(right->type == NodeType::VAR || right->type == NodeType::NUMBER))
      return false;
    return true;
  };
  assignArithOpTile.generate_L2_code = [](TreeNode* node) {
    TreeNode* dst = node;
    TreeNode* op = node->children[0]->children[0].get();
    TreeNode* left = op->children[0].get();
    TreeNode* right = op->children[1].get();

    std::vector<std::string> L2Instructions;

    auto opToString = [](NodeType t) -> const char* {
      switch (t) {
        case NodeType::ADD:
          return "+=";
        case NodeType::SUB:
          return "-=";
        case NodeType::MUL:
          return "*=";
        case NodeType::AND:
          return "&=";
        case NodeType::LSHIFT:
          return "<<=";
        case NodeType::RSHIFT:
          return ">>=";
        default:
          throw std::runtime_error("Not a valid Arithmetic Op");
      }
    };

    auto isCommutative = [](NodeType t) -> bool {
      return t == NodeType::ADD || t == NodeType::MUL || t == NodeType::AND;
    };

    auto operandString = [](TreeNode* n) -> std::string {
      if (n->type == NodeType::VAR) return n->name;
      if (n->type == NodeType::NUMBER) return std::to_string(n->value);
      throw std::runtime_error("Expected VAR/NUMBER");
    };

    auto evalConstant = [&](long long a, long long b) {
      switch (op->type) {
        case NodeType::ADD:
          return a + b;
        case NodeType::SUB:
          return a - b;
        case NodeType::MUL:
          return a * b;
        case NodeType::AND:
          return a & b;
        case NodeType::LSHIFT:
          return a << b;
        case NodeType::RSHIFT:
          return a >> b;
        default:
          throw std::runtime_error("Not a valid Arithmetic Op");
      }
    };

    auto emitInPlaceStr = [&](const std::string& rhs) {
      std::ostringstream out;
      out << dst->name << " " << opToString(op->type) << " " << rhs;
      L2Instructions.push_back(out.str());
    };

    auto emitMoveStr = [&](const std::string& src) {
      std::ostringstream out;
      out << dst->name << " <- " << src;
      L2Instructions.push_back(out.str());
    };

    auto emitInPlace = [&](TreeNode* rhs) {
      emitInPlaceStr(operandString(rhs));
    };
    auto emitMove = [&](TreeNode* src) { emitMoveStr(operandString(src)); };

    auto dstIsVarNamed = [&](TreeNode* n) -> bool {
      return n->type == NodeType::VAR && n->name == dst->name;
    };

    const bool leftIsDst = dstIsVarNamed(left);
    const bool rightIsDst = dstIsVarNamed(right);

    // Case 1: constant folding
    if (left->type == NodeType::NUMBER && right->type == NodeType::NUMBER) {
      long long newValue = evalConstant(left->value, right->value);
      std::ostringstream out;
      out << dst->name << " <- " << newValue;
      L2Instructions.push_back(out.str());
      return L2Instructions;
    }

    // Case 2: dst is left var => dst op= right  (always safe)
    if (leftIsDst) {
      emitInPlace(right);
      return L2Instructions;
    }

    // Case 3: dst is right var and op commutative => dst op= left  (safe)
    if (rightIsDst && isCommutative(op->type)) {
      emitInPlace(left);
      return L2Instructions;
    }

    // Case 4: dst is right var and op NOT commutative
    if (rightIsDst && !isCommutative(op->type)) {
      // If a function uses the var scratch, this will error
      const std::string SCR = "%scratch";
      L2Instructions.push_back(SCR + " <- " + dst->name);
      emitMove(left);
      emitInPlaceStr(SCR);
      return L2Instructions;
    }

    // Case 5: general case => dst <- left ; dst op= right
    emitMove(left);
    emitInPlace(right);
    return L2Instructions;
  };
  assignArithOpTile.find_children = [](TreeNode* node) {
    TreeNode* dst = node;
    TreeNode* op = node->children[0]->children[0].get();
    TreeNode* left = op->children[0].get();
    TreeNode* right = op->children[1].get();
    std::vector<TreeNode*> children;
    if (!left->children.empty()) children.push_back(left);
    if (!right->children.empty()) children.push_back(right);
    return children;
  };

  TreeTile assignCompareOpTile;
  assignCompareOpTile.numNodes = 5;
  assignCompareOpTile.numInstructions = 1;
  assignCompareOpTile.match = [](TreeNode* node) {
    if (!node) return false;
    if (node->type != NodeType::VAR) return false;
    if (node->children.size() != 1) return false;
    node = node->children[0].get();
    if (node->type != NodeType::ARROW) return false;
    if (node->children.size() != 1) return false;
    node = node->children[0].get();
    if (!(node->type == NodeType::LT || node->type == NodeType::LE ||
          node->type == NodeType::EQ || node->type == NodeType::GE ||
          node->type == NodeType::GT)) {
      return false;
    }
    if (node->children.size() != 2) return false;
    TreeNode* left = node->children[0].get();
    TreeNode* right = node->children[1].get();
    if (!(left->type == NodeType::VAR || left->type == NodeType::NUMBER))
      return false;
    if (!(right->type == NodeType::VAR || right->type == NodeType::NUMBER))
      return false;
    return true;
  };
  assignCompareOpTile.generate_L2_code = [](TreeNode* node) {
    TreeNode* dst = node;
    TreeNode* op = node->children[0]->children[0].get();
    TreeNode* left = op->children[0].get();
    TreeNode* right = op->children[1].get();
    std::vector<std::string> L2Instructions;
    std::ostringstream out;
    out << dst->name << " <- ";
    if (op->type == NodeType::GE || op->type == NodeType::GT) {
      if (right->type == NodeType::VAR) {
        out << right->name;
      } else {
        out << right->value;
      }
      if (op->type == NodeType::GE) {
        out << " <= ";
      } else {
        out << " < ";
      }
      if (left->type == NodeType::VAR) {
        out << left->name;
      } else {
        out << left->value;
      }
    } else {
      if (left->type == NodeType::VAR) {
        out << left->name;
      } else {
        out << left->value;
      }
      if (op->type == NodeType::LT) {
        out << " < ";
      } else if (op->type == NodeType::LE) {
        out << " <= ";
      } else if (op->type == NodeType::EQ) {
        out << " = ";
      }
      if (right->type == NodeType::VAR) {
        out << right->name;
      } else {
        out << right->value;
      }
    }
    L2Instructions.push_back(out.str());
    return L2Instructions;
  };
  assignCompareOpTile.find_children = [](TreeNode* node) {
    TreeNode* dst = node;
    TreeNode* op = node->children[0]->children[0].get();
    TreeNode* left = op->children[0].get();
    TreeNode* right = op->children[1].get();
    std::vector<TreeNode*> children;
    if (!left->children.empty()) children.push_back(left);
    if (!right->children.empty()) children.push_back(right);
    return children;
  };

  TreeTile loadTile;
  loadTile.numInstructions = 1;
  loadTile.numNodes = 4;
  loadTile.match = [](TreeNode* node) {
    if (!node) return false;
    if (node->type != NodeType::VAR) return false;
    if (node->children.size() != 1) return false;
    node = node->children[0].get();
    if (node->type != NodeType::ARROW) return false;
    if (node->children.size() != 1) return false;
    node = node->children[0].get();
    if (node->type != NodeType::LOAD) return false;
    if (node->children.size() != 1) return false;
    node = node->children[0].get();
    if (node->type != NodeType::VAR) return false;
    return true;
  };
  loadTile.generate_L2_code = [](TreeNode* node) {
    std::vector<std::string> L2Instructions;
    std::ostringstream out;
    TreeNode* dst = node;
    TreeNode* src = node->children[0]->children[0]->children[0].get();
    out << dst->name;
    out << " <- mem ";
    out << src->name;
    out << " 0";
    L2Instructions.push_back(out.str());
    return L2Instructions;
  };
  loadTile.find_children = [](TreeNode* node) {
    std::vector<TreeNode*> children;
    TreeNode* src = node->children[0]->children[0]->children[0].get();
    if (!src->children.empty()) {
      children.push_back(src);
    }
    return children;
  };

  TreeTile storeTile;
  storeTile.numInstructions = 1;
  storeTile.numNodes = 4;
  storeTile.match = [](TreeNode* node) {
    if (!node) return false;
    if (node->type != NodeType::STORE) return false;
    if (node->children.size() != 1) return false;
    node = node->children[0].get();
    if (node->type != NodeType::VAR) return false;
    if (node->children.size() != 1) return false;
    node = node->children[0].get();
    if (node->type != NodeType::ARROW) return false;
    if (node->children.size() != 1) return false;
    node = node->children[0].get();
    if (!(node->type == NodeType::VAR || node->type == NodeType::NUMBER ||
          node->type == NodeType::LABEL))
      return false;
    return true;
  };
  storeTile.generate_L2_code = [](TreeNode* node) {
    std::vector<std::string> L2Instructions;
    std::ostringstream out;
    TreeNode* dst = node->children[0].get();
    TreeNode* src = dst->children[0]->children[0].get();
    out << "mem ";
    out << dst->name;
    out << " 0 <- ";
    if (src->type == NodeType::LABEL || src->type == NodeType::VAR) {
      out << src->name;
    } else {
      out << src->value;
    }
    L2Instructions.push_back(out.str());
    return L2Instructions;
  };
  storeTile.find_children = [](TreeNode* node) {
    std::vector<TreeNode*> children;
    TreeNode* src = node->children[0]->children[0]->children[0].get();
    if (!src->children.empty()) {
      children.push_back(src);
    }
    return children;
  };

  TreeTile returnTile;
  returnTile.numInstructions = 1;
  returnTile.numNodes = 1;
  returnTile.match = [](TreeNode* node) {
    if (!node) return false;
    if (!node->children.empty()) return false;
    return node->type == NodeType::RETURN;
  };
  returnTile.generate_L2_code = [](TreeNode* node) {
    return std::vector<std::string>{"return"};
  };
  returnTile.find_children = [](TreeNode* node) {
    return std::vector<TreeNode*>();
  };

  TreeTile returnValTile;
  returnValTile.numInstructions = 1;
  returnValTile.numNodes = 2;
  returnValTile.match = [](TreeNode* node) {
    if (!node) return false;
    if (node->type != NodeType::RETURN) return false;
    if (node->children.size() != 1) return false;
    node = node->children[0].get();
    if (!(node->type == NodeType::NUMBER || node->type == NodeType::VAR)) {
      return false;
    }
    return true;
  };
  returnValTile.generate_L2_code = [](TreeNode* node) {
    std::vector<std::string> L2Instructions;
    std::ostringstream out;
    node = node->children[0].get();
    if (node->type == NodeType::NUMBER) {
      out << "rax <- " << node->value << "\n";
    } else {
      out << "rax <- " << node->name << "\n";
    }
    out << "return";
    L2Instructions.push_back(out.str());
    return L2Instructions;
  };
  returnValTile.find_children = [](TreeNode* node) {
    std::vector<TreeNode*> children;
    node = node->children[0].get();
    if (!node->children.empty()) children.push_back(node);
    return children;
  };

  TreeTile branchTile;
  branchTile.numInstructions = 1;
  branchTile.numNodes = 2;
  branchTile.match = [](TreeNode* node) {
    if (!node) return false;
    if (node->type != NodeType::BR) return false;
    if (node->children.size() != 1) return false;
    node = node->children[0].get();
    if (node->type != NodeType::LABEL) return false;
    return true;
  };
  branchTile.generate_L2_code = [](TreeNode* node) {
    std::vector<std::string> L2Instructions;
    std::ostringstream out;
    out << "goto ";
    TreeNode* label = node->children[0].get();
    out << label->name;
    L2Instructions.push_back(out.str());
    return L2Instructions;
  };
  branchTile.find_children = [](TreeNode* node) {
    std::vector<TreeNode*> children;
    node = node->children[0].get();
    if (!node->children.empty()) children.push_back(node);
    return children;
  };

  TreeTile compareBranchTile;
  compareBranchTile.numInstructions = 1;
  compareBranchTile.numNodes = 7;
  compareBranchTile.match = [](TreeNode* node) {
    if (!node || node->type != NodeType::BR || node->children.size() != 1) {
      return false;
    }
    TreeNode* cond = node->children[0].get();
    if (cond->type != NodeType::VAR) {
      return false;
    }

    TreeNode* arrow = nullptr;
    TreeNode* label = nullptr;
    for (auto& child : cond->children) {
      if (child->type == NodeType::ARROW) {
        if (arrow != nullptr) return false;
        arrow = child.get();
      } else if (child->type == NodeType::LABEL) {
        if (label != nullptr) return false;
        label = child.get();
      } else {
        return false;
      }
    }
    if (arrow == nullptr || label == nullptr || arrow->children.size() != 1) {
      return false;
    }

    TreeNode* cmp = arrow->children[0].get();
    if (!(cmp->type == NodeType::LT || cmp->type == NodeType::LE ||
          cmp->type == NodeType::EQ || cmp->type == NodeType::GE ||
          cmp->type == NodeType::GT)) {
      return false;
    }
    if (cmp->children.size() != 2) return false;
    TreeNode* left = cmp->children[0].get();
    TreeNode* right = cmp->children[1].get();
    if (!(left->type == NodeType::VAR || left->type == NodeType::NUMBER)) {
      return false;
    }
    if (!(right->type == NodeType::VAR || right->type == NodeType::NUMBER)) {
      return false;
    }
    return true;
  };
  compareBranchTile.generate_L2_code = [](TreeNode* node) {
    std::vector<std::string> L2Instructions;
    TreeNode* cond = node->children[0].get();
    TreeNode* arrow = nullptr;
    TreeNode* label = nullptr;
    for (auto& child : cond->children) {
      if (child->type == NodeType::ARROW) {
        arrow = child.get();
      } else if (child->type == NodeType::LABEL) {
        label = child.get();
      }
    }
    if (!arrow || !label || arrow->children.size() != 1) {
      throw std::runtime_error("Malformed compareBranch tile");
    }

    TreeNode* cmp = arrow->children[0].get();
    TreeNode* left = cmp->children[0].get();
    TreeNode* right = cmp->children[1].get();

    auto operand_to_string = [](TreeNode* operand) -> std::string {
      if (operand->type == NodeType::VAR) {
        return operand->name;
      }
      if (operand->type == NodeType::NUMBER) {
        return std::to_string(operand->value);
      }
      throw std::runtime_error("Expected VAR/NUMBER in compareBranch tile");
    };

    std::ostringstream out;
    out << "cjump ";
    if (cmp->type == NodeType::GE || cmp->type == NodeType::GT) {
      out << operand_to_string(right);
      out << (cmp->type == NodeType::GE ? " <= " : " < ");
      out << operand_to_string(left);
    } else {
      out << operand_to_string(left);
      if (cmp->type == NodeType::LT) {
        out << " < ";
      } else if (cmp->type == NodeType::LE) {
        out << " <= ";
      } else {
        out << " = ";
      }
      out << operand_to_string(right);
    }
    out << " " << label->name;
    L2Instructions.push_back(out.str());
    return L2Instructions;
  };
  compareBranchTile.find_children = [](TreeNode* node) {
    std::vector<TreeNode*> children;
    TreeNode* cond = node->children[0].get();
    TreeNode* arrow = nullptr;
    for (auto& child : cond->children) {
      if (child->type == NodeType::ARROW) {
        arrow = child.get();
        break;
      }
    }
    if (!arrow || arrow->children.size() != 1) {
      return children;
    }
    TreeNode* cmp = arrow->children[0].get();
    if (cmp->children.size() != 2) {
      return children;
    }
    TreeNode* left = cmp->children[0].get();
    TreeNode* right = cmp->children[1].get();
    if (!left->children.empty()) children.push_back(left);
    if (!right->children.empty()) children.push_back(right);
    return children;
  };

  TreeTile condBranchTile;
  condBranchTile.numInstructions = 1;
  condBranchTile.numNodes = 3;
  condBranchTile.match = [](TreeNode* node) {
    if (!node) return false;
    if (node->type != NodeType::BR) return false;
    if (node->children.size() != 1) return false;
    node = node->children[0].get();
    if (!(node->type == NodeType::VAR || node->type == NodeType::NUMBER))
      return false;
    if (node->children.size() != 1) return false;
    node = node->children[0].get();
    if (node->type != NodeType::LABEL) return false;
    return true;
  };
  condBranchTile.generate_L2_code = [](TreeNode* node) {
    std::vector<std::string> L2Instructions;
    std::ostringstream out;
    out << "cjump ";
    TreeNode* condition = node->children[0].get();
    TreeNode* label = condition->children[0].get();
    if (condition->type == NodeType::VAR) {
      out << condition->name << " = 1 ";
    } else {
      out << condition->value << " = 1 ";
    }
    out << label->name;
    L2Instructions.push_back(out.str());
    return L2Instructions;
  };
  condBranchTile.find_children = [](TreeNode* node) {
    std::vector<TreeNode*> children;
    node = node->children[0]->children[0].get();
    if (!node->children.empty()) children.push_back(node);
    return children;
  };

  auto isValidE = [](TreeNode* n) -> bool {
    return n->type == NodeType::NUMBER &&
           (n->value == 1 || n->value == 2 || n->value == 4 || n->value == 8);
  };
  auto isLeafOperand = [](TreeNode* n) -> bool {
    return (n->type == NodeType::VAR || n->type == NodeType::NUMBER) &&
           n->children.empty();
  };
  auto isLeafVar = [](TreeNode* n) -> bool {
    return n->type == NodeType::VAR && n->children.empty();
  };
  auto isMergedMulWithE = [&](TreeNode* n) -> bool {
    if (n->type != NodeType::VAR || n->children.size() != 1) return false;
    TreeNode* arrow = n->children[0].get();
    if (arrow->type != NodeType::ARROW || arrow->children.size() != 1)
      return false;
    TreeNode* mul = arrow->children[0].get();
    if (mul->type != NodeType::MUL || mul->children.size() != 2) return false;
    TreeNode* mL = mul->children[0].get();
    TreeNode* mR = mul->children[1].get();
    return (isValidE(mR) && isLeafVar(mL)) ||
           (isValidE(mL) && isLeafVar(mR));
  };

  TreeTile leaTile;
  leaTile.numNodes = 9;
  leaTile.numInstructions = 1;
  leaTile.match = [&isMergedMulWithE, &isLeafVar](TreeNode* node) {
    if (!node || node->type != NodeType::VAR) return false;
    if (node->children.size() != 1) return false;
    TreeNode* arrow = node->children[0].get();
    if (arrow->type != NodeType::ARROW || arrow->children.size() != 1)
      return false;
    TreeNode* add = arrow->children[0].get();
    if (add->type != NodeType::ADD || add->children.size() != 2) return false;
    TreeNode* left = add->children[0].get();
    TreeNode* right = add->children[1].get();
    return (isMergedMulWithE(left) && isLeafVar(right)) ||
           (isMergedMulWithE(right) && isLeafVar(left));
  };
  leaTile.generate_L2_code = [&isMergedMulWithE, &isValidE](TreeNode* node) {
    TreeNode* dst = node;
    TreeNode* add = node->children[0]->children[0].get();
    TreeNode* left = add->children[0].get();
    TreeNode* right = add->children[1].get();

    TreeNode* mulSide = isMergedMulWithE(left) ? left : right;
    TreeNode* addend = isMergedMulWithE(left) ? right : left;
    TreeNode* mul = mulSide->children[0]->children[0].get();
    TreeNode* mL = mul->children[0].get();
    TreeNode* mR = mul->children[1].get();
    TreeNode* mulOperand = isValidE(mR) ? mL : mR;
    TreeNode* eNode = isValidE(mR) ? mR : mL;

    auto opStr = [](TreeNode* n) -> std::string {
      return n->type == NodeType::VAR ? n->name : std::to_string(n->value);
    };

    std::ostringstream out;
    out << dst->name << " @ " << opStr(addend) << " " << opStr(mulOperand)
        << " " << eNode->value;
    return std::vector<std::string>{out.str()};
  };
  leaTile.find_children = [&isMergedMulWithE, &isValidE](TreeNode* node) {
    TreeNode* add = node->children[0]->children[0].get();
    TreeNode* left = add->children[0].get();
    TreeNode* right = add->children[1].get();
    TreeNode* mulSide = isMergedMulWithE(left) ? left : right;
    TreeNode* addend = isMergedMulWithE(left) ? right : left;
    TreeNode* mul = mulSide->children[0]->children[0].get();
    TreeNode* mL = mul->children[0].get();
    TreeNode* mR = mul->children[1].get();
    TreeNode* mulOperand = isValidE(mR) ? mL : mR;

    std::vector<TreeNode*> children;
    if (!addend->children.empty()) children.push_back(addend);
    if (!mulOperand->children.empty()) children.push_back(mulOperand);
    return children;
  };

  treeTiles.push_back(std::move(leaTile));
  treeTiles.push_back(std::move(assignVarNumLabel));
  treeTiles.push_back(std::move(assignArithOpTile));
  treeTiles.push_back(std::move(assignCompareOpTile));
  treeTiles.push_back(std::move(loadTile));
  treeTiles.push_back(std::move(storeTile));
  treeTiles.push_back(std::move(returnTile));
  treeTiles.push_back(std::move(returnValTile));
  treeTiles.push_back(std::move(branchTile));
  treeTiles.push_back(std::move(compareBranchTile));
  treeTiles.push_back(std::move(condBranchTile));

  std::sort(treeTiles.begin(), treeTiles.end(),
            [](const auto& a, const auto& b) {
              if (a.numNodes != b.numNodes) return a.numNodes > b.numNodes;
              return a.numInstructions < b.numInstructions;
            });
}

void InstructionSelector::load_and_identify_contexts(
    const std::vector<Instruction>& instructions) {
  Context context;
  for (const Instruction& instruction : instructions) {
    std::visit(
        [this, &context](auto&& instruction) {
          using T = std::decay_t<decltype(instruction)>;
          if constexpr (std::is_same_v<T, LabelInstruction> ||
                        std::is_same_v<T, CallInstruction> ||
                        std::is_same_v<T, AssignCallInstruction>) {
            if (!context.instructions.empty()) {
              this->functionContexts.back().partitions.push_back(
                  std::move(context));
            }
            context = Context{};
            this->functionContexts.back().partitions.push_back(instruction);
          } else {
            context.instructions.push_back(instruction);
            if constexpr (std::is_same_v<T, BranchInstruction> ||
                          std::is_same_v<T, ConditionalBranchInstruction> ||
                          std::is_same_v<T, ReturnInstruction> ||
                          std::is_same_v<T, ReturnValInstruction>) {
              this->functionContexts.back().partitions.push_back(
                  std::move(context));
              context = Context{};
            }
          }
        },
        instruction);
  }
  if (!context.instructions.empty()) {
    this->functionContexts.back().partitions.push_back(std::move(context));
  }
}

std::unique_ptr<TreeNode> InstructionSelector::generate_tree(
    const Instruction& instruction) {
  return std::visit(
      [this](auto&& instruction) {
        using T = std::decay_t<decltype(instruction)>;

        if constexpr (std::is_same_v<T, AssignInstruction>) {
          Var dst = instruction.get_dst();
          std::variant<Var, Number, Label, ArithmeticOp, CompareOp> src =
              instruction.get_src();
          std::unique_ptr<TreeNode> dstNode = factory.make_var(dst);

          std::unique_ptr<TreeNode> srcNode = std::visit(
              [this](auto&& src) {
                using V = std::decay_t<decltype(src)>;
                if constexpr (std::is_same_v<V, Var>) {
                  return this->factory.make_var(src);
                } else if constexpr (std::is_same_v<V, Number>) {
                  return this->factory.make_number(src);
                } else if constexpr (std::is_same_v<V, Label>) {
                  return this->factory.make_label(src);
                } else if constexpr (std::is_same_v<V, ArithmeticOp>) {
                  return this->factory.make_arithmetic_op(src);
                } else if constexpr (std::is_same_v<V, CompareOp>) {
                  return this->factory.make_compare_op(src);
                } else {
                  throw std::runtime_error("Invalid variant");
                  return std::unique_ptr<TreeNode>(nullptr);
                }
              },
              src);

          std::unique_ptr<TreeNode> arrowNode = factory.make_arrow();
          arrowNode->children.push_back(std::move(srcNode));
          dstNode->children.push_back(std::move(arrowNode));
          return dstNode;
        } else if constexpr (std::is_same_v<T, LoadInstruction>) {
          Var dst = instruction.get_dst();
          Var src = instruction.get_src();
          std::unique_ptr<TreeNode> dstNode = factory.make_var(dst);
          std::unique_ptr<TreeNode> srcNode = factory.make_var(src);
          std::unique_ptr<TreeNode> loadNode = factory.make_load();
          std::unique_ptr<TreeNode> arrowNode = factory.make_arrow();

          loadNode->children.push_back(std::move(srcNode));
          arrowNode->children.push_back(std::move(loadNode));
          dstNode->children.push_back(std::move(arrowNode));
          return dstNode;
        } else if constexpr (std::is_same_v<T, StoreInstruction>) {
          Var dst = instruction.get_dst();
          std::variant<Var, Number, Label> src = instruction.get_src();
          std::unique_ptr<TreeNode> dstNode = factory.make_var(dst);
          std::unique_ptr<TreeNode> srcNode = std::visit(
              [this](auto&& src) {
                using V = std::decay_t<decltype(src)>;
                if constexpr (std::is_same_v<V, Var>) {
                  return this->factory.make_var(src);
                } else if constexpr (std::is_same_v<V, Number>) {
                  return this->factory.make_number(src);
                } else if constexpr (std::is_same_v<V, Label>) {
                  return this->factory.make_label(src);
                } else {
                  throw std::runtime_error("Invalid variant");
                  return std::unique_ptr<TreeNode>(nullptr);
                }
              },
              src);

          std::unique_ptr<TreeNode> storeNode = factory.make_store();
          std::unique_ptr<TreeNode> arrowNode = factory.make_arrow();

          arrowNode->children.push_back(std::move(srcNode));
          dstNode->children.push_back(std::move(arrowNode));
          storeNode->children.push_back(std::move(dstNode));
          return storeNode;
        } else if constexpr (std::is_same_v<T, ReturnInstruction>) {
          std::unique_ptr<TreeNode> returnNode = std::make_unique<TreeNode>();
          returnNode->type = NodeType::RETURN;
          return returnNode;
        } else if constexpr (std::is_same_v<T, ReturnValInstruction>) {
          std::unique_ptr<TreeNode> returnNode = std::make_unique<TreeNode>();
          returnNode->type = NodeType::RETURN;
          Operand returnVal = instruction.get_returnVal();
          std::unique_ptr<TreeNode> returnValNode = std::visit(
              [this](auto&& returnVal) {
                using V = std::decay_t<decltype(returnVal)>;
                if constexpr (std::is_same_v<V, Var>) {
                  return this->factory.make_var(returnVal);
                } else if constexpr (std::is_same_v<V, Number>) {
                  return this->factory.make_number(returnVal);
                } else {
                  throw std::runtime_error("Invalid Operand");
                  return std::unique_ptr<TreeNode>(nullptr);
                }
              },
              returnVal);
          returnNode->children.push_back(std::move(returnValNode));
          return returnNode;
        } else if constexpr (std::is_same_v<T, BranchInstruction>) {
          Label target = instruction.get_target();
          std::unique_ptr<TreeNode> targetNode = factory.make_label(target);
          std::unique_ptr<TreeNode> breakNode = factory.make_break();
          breakNode->children.push_back(std::move(targetNode));
          return breakNode;
        } else if constexpr (std::is_same_v<T, ConditionalBranchInstruction>) {
          Label target = instruction.get_target();
          std::unique_ptr<TreeNode> targetNode = factory.make_label(target);
          std::unique_ptr<TreeNode> breakNode = factory.make_break();
          Operand condition = instruction.get_condition();
          std::unique_ptr<TreeNode> conditionNode = std::visit(
              [this](auto&& condition) {
                using V = std::decay_t<decltype(condition)>;
                if constexpr (std::is_same_v<V, Var>) {
                  return this->factory.make_var(condition);
                } else if constexpr (std::is_same_v<V, Number>) {
                  return this->factory.make_number(condition);
                } else {
                  throw std::runtime_error("Invalid Operand");
                  return std::unique_ptr<TreeNode>(nullptr);
                }
              },
              condition);
          conditionNode->children.push_back(std::move(targetNode));
          breakNode->children.push_back(std::move(conditionNode));
          return breakNode;
        } else if constexpr (std::is_same_v<T, LabelInstruction>) {
          throw std::runtime_error(
              "LabelInstruction should not be in a context");
          return std::unique_ptr<TreeNode>(nullptr);
        } else if constexpr (std::is_same_v<T, CallInstruction>) {
          throw std::runtime_error(
              "CallInstruction should not be in a context");
          return std::unique_ptr<TreeNode>(nullptr);
        } else if constexpr (std::is_same_v<T, AssignCallInstruction>) {
          throw std::runtime_error(
              "AssignCallInstruction should not be in a context");
          return std::unique_ptr<TreeNode>(nullptr);
        } else {
          throw std::runtime_error("Instruction type error");
          return std::unique_ptr<TreeNode>(nullptr);
        }
      },
      instruction);
}

void InstructionSelector::generate_trees() {
  for (auto& partition : functionContexts.back().partitions) {
    if (std::holds_alternative<LabelInstruction>(partition) ||
        std::holds_alternative<CallInstruction>(partition) ||
        std::holds_alternative<AssignCallInstruction>(partition)) {
      continue;
    }
    Context& context = std::get<Context>(partition);
    for (const Instruction& instruction : context.instructions) {
      context.trees.push_back(generate_tree(instruction));
    }
  }
}

std::string InstructionSelector::get_defined_var(TreeNode* node) {
  if (node && node->type == NodeType::VAR) {
    return node->name;
  }
  return "";
}

std::vector<std::string> InstructionSelector::get_used_vars(TreeNode* node) {
  std::vector<std::string> vars;
  std::function<void(TreeNode*, bool)> dfs = [&](TreeNode* n, bool isRoot) {
    if (!n) return;
    if (n->type == NodeType::VAR && !isRoot) {
      vars.push_back(n->name);
    }
    for (auto& child : n->children) {
      dfs(child.get(), false);
    }
  };
  dfs(node, true);
  return vars;
}

bool InstructionSelector::involves_memory(TreeNode* node) {
  if (!node) return false;
  if (node->type == NodeType::LOAD || node->type == NodeType::STORE)
    return true;
  for (auto& child : node->children) {
    if (involves_memory(child.get())) return true;
  }
  return false;
}

bool InstructionSelector::can_merge_trees(
    std::vector<std::unique_ptr<TreeNode>>& trees, int i, int j,
    std::string definedVar) {
  auto is_compare_definition_tree = [&](TreeNode* root,
                                        const std::string& varName) -> bool {
    if (!root || root->type != NodeType::VAR || root->name != varName ||
        root->children.size() != 1) {
      return false;
    }
    TreeNode* arrow = root->children[0].get();
    if (arrow->type != NodeType::ARROW || arrow->children.size() != 1) {
      return false;
    }
    TreeNode* rhs = arrow->children[0].get();
    return rhs->type == NodeType::LT || rhs->type == NodeType::LE ||
           rhs->type == NodeType::EQ || rhs->type == NodeType::GE ||
           rhs->type == NodeType::GT;
  };

  auto tree_defines_var = [&](TreeNode* root, const std::string& varName) {
    std::function<bool(TreeNode*)> dfs = [&](TreeNode* n) -> bool {
      if (!n) return false;
      // Assignment trees are represented as: VAR -> ARROW -> ...
      if (n->type == NodeType::VAR && n->name == varName &&
          !n->children.empty() && n->children[0]->type == NodeType::ARROW) {
        return true;
      }
      for (auto& child : n->children) {
        if (dfs(child.get())) return true;
      }
      return false;
    };
    return dfs(root);
  };

  const bool defines_compare =
      is_compare_definition_tree(trees[i].get(), definedVar);

  // Structural check: definedVar must appear as a leaf VAR in tree j,
  // or as a branch condition VAR carrying a label child.
  std::function<bool(TreeNode*)> hasVarLeaf = [&](TreeNode* n) -> bool {
    if (!n) return false;
    if (n->type == NodeType::VAR && n->name == definedVar && n->children.empty())
      return true;
    if (defines_compare && n->type == NodeType::VAR && n->name == definedVar &&
        n->children.size() == 1 && n->children[0]->type == NodeType::LABEL)
      return true;
    for (auto& child : n->children) {
      if (hasVarLeaf(child.get())) return true;
    }
    return false;
  };
  if (!hasVarLeaf(trees[j].get())) return false;

  // Condition A: definedVar is only used by tree j (no other tree in any
  // context uses it)
  for (int k = 0; k < (int)trees.size(); k++) {
    if (k == i || k == j) continue;
    auto used = get_used_vars(trees[k].get());
    if (std::find(used.begin(), used.end(), definedVar) != used.end())
      return false;
  }
  // Also check all other partitions in the function for uses of definedVar
  for (auto& partition : functionContexts.back().partitions) {
    if (std::holds_alternative<Context>(partition)) {
      Context& otherCtx = std::get<Context>(partition);
      // Skip if this is the same context (already checked above)
      if (&otherCtx.trees == &trees) continue;
      for (auto& tree : otherCtx.trees) {
        auto used = get_used_vars(tree.get());
        if (std::find(used.begin(), used.end(), definedVar) != used.end())
          return false;
      }
      continue;
    }

    auto operand_uses_var = [&](const Operand& operand) -> bool {
      return std::visit(
          [&](auto&& item) -> bool {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::is_same_v<T, Var>) {
              return item.name == definedVar;
            } else {
              return false;
            }
          },
          operand);
    };

    auto callee_uses_var = [&](const Callee& callee) -> bool {
      return std::visit(
          [&](auto&& item) -> bool {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::is_same_v<T, Var>) {
              return item.name == definedVar;
            } else {
              return false;
            }
          },
          callee);
    };

    if (std::holds_alternative<CallInstruction>(partition)) {
      const CallInstruction& call = std::get<CallInstruction>(partition);
      if (callee_uses_var(call.get_callee())) return false;
      for (const Operand& arg : call.get_args()) {
        if (operand_uses_var(arg)) return false;
      }
      continue;
    }

    if (std::holds_alternative<AssignCallInstruction>(partition)) {
      const AssignCallInstruction& call =
          std::get<AssignCallInstruction>(partition);
      if (callee_uses_var(call.get_callee())) return false;
      for (const Operand& arg : call.get_args()) {
        if (operand_uses_var(arg)) return false;
      }
    }
  }

  // Condition B: no tree between i and j defines a variable that tree i uses
  auto i_used_vars = get_used_vars(trees[i].get());
  for (int k = i + 1; k < j; k++) {
    std::string k_defined = get_defined_var(trees[k].get());
    // Do not move a definition across an intervening redefinition
    // of the same variable; that changes program semantics.
    if (!k_defined.empty() && k_defined == definedVar) {
      return false;
    }
    // Also block if the redefinition exists inside a merged subtree.
    if (tree_defines_var(trees[k].get(), definedVar)) {
      return false;
    }
    if (!k_defined.empty()) {
      if (std::find(i_used_vars.begin(), i_used_vars.end(), k_defined) !=
          i_used_vars.end())
        return false;
    }
  }

  // Memory condition: if tree i involves memory, no memory instruction between
  // i and j
  if (involves_memory(trees[i].get())) {
    for (int k = i + 1; k < j; k++) {
      if (involves_memory(trees[k].get())) return false;
    }
  }

  return true;
}

void InstructionSelector::merge_tree(std::unique_ptr<TreeNode>& top,
                                     std::unique_ptr<TreeNode>& bottom,
                                     std::string definedVar) {
  std::function<bool(std::unique_ptr<TreeNode>&)> replace =
      [&](std::unique_ptr<TreeNode>& node) -> bool {
    for (auto& child : node->children) {
      const bool is_leaf_var =
          child->type == NodeType::VAR && child->name == definedVar &&
          child->children.empty();
      const bool is_branch_condition_var =
          child->type == NodeType::VAR && child->name == definedVar &&
          child->children.size() == 1 &&
          child->children[0]->type == NodeType::LABEL;

      if (is_leaf_var || is_branch_condition_var) {
        std::vector<std::unique_ptr<TreeNode>> carried_children;
        for (auto& old_child : child->children) {
          carried_children.push_back(std::move(old_child));
        }
        child = std::move(bottom);
        for (auto& carried : carried_children) {
          child->children.push_back(std::move(carried));
        }
        return true;
      }
      if (replace(child)) return true;
    }
    return false;
  };

  replace(top);
}

void InstructionSelector::merge_trees() {
  for (auto& partition : functionContexts.back().partitions) {
    if (!std::holds_alternative<Context>(partition)) continue;
    Context& context = std::get<Context>(partition);
    bool changed = true;
    while (changed) {
      changed = false;
      for (int j = 1; j < context.trees.size(); ++j) {
        std::string j_defined_var = get_defined_var(context.trees[j].get());
        std::vector<std::string> j_used_vars =
            get_used_vars(context.trees[j].get());
        // Conservative policy: only merge adjacent producer->consumer pairs.
        // This avoids unsafe long-range motion/reordering while preserving
        // straightforward local merge opportunities.
        int i = j - 1;
        std::string i_defined_var = get_defined_var(context.trees[i].get());
        if (std::find(j_used_vars.begin(), j_used_vars.end(),
                      i_defined_var) != j_used_vars.end() &&
            can_merge_trees(context.trees, i, j, i_defined_var)) {
          merge_tree(context.trees[j], context.trees[i], i_defined_var);
          context.trees.erase(context.trees.begin() + i);
          changed = true;
          break;
        }
      }
    }
  }
}

void InstructionSelector::tile_trees() {
  for (auto& partition : functionContexts.back().partitions) {
    if (!std::holds_alternative<Context>(partition)) continue;
    Context& context = std::get<Context>(partition);
    for (auto& tree : context.trees) {
      std::stack<TreeNode*> toVisit;
      std::stack<std::string> treeL2Code;
      toVisit.push(tree.get());
      while (!toVisit.empty()) {
        TreeNode* curTreeNode = toVisit.top();
        toVisit.pop();
        bool matched = false;
        for (const auto& treeTile : treeTiles) {
          if (treeTile.match(curTreeNode)) {
            matched = true;
            std::vector<std::string> generatedL2Code =
                treeTile.generate_L2_code(curTreeNode);
            // This is so the ordering is correct for tiles that generate more
            // than 1 instruction
            std::reverse(generatedL2Code.begin(), generatedL2Code.end());
            for (const auto& code : generatedL2Code) {
              treeL2Code.push(code);
            }
            std::vector<TreeNode*> childrenTreeNodes =
                treeTile.find_children(curTreeNode);
            for (const auto childTreeNode : childrenTreeNodes) {
              toVisit.push(childTreeNode);
            }
            break;
          }
        }
        if (!matched) throw std::runtime_error("Insuffient tiles");
      }
      while (!treeL2Code.empty()) {
        context.L2Code.push_back(treeL2Code.top());
        treeL2Code.pop();
      }
    }
  }
}

std::string InstructionSelector::emit_L2_instructions() {
  std::ostringstream out;
  out << "(@main\n";

  int returnInstructionCounter = 0;

  for (const FunctionContext& functionContext : functionContexts) {
    out << "(" << functionContext.functionName << " " << functionContext.numArgs
        << "\n";

    std::vector<std::string> argRegisters = {"rdi", "rsi", "rdx",
                                             "rcx", "r8",  "r9"};

    for (int i = 0; i < std::min(functionContext.numArgs, 6); ++i) {
      out << functionContext.vars[i].to_string() << " <- " << argRegisters[i]
          << "\n";
    }

    if (functionContext.numArgs > 6) {
      for (int i = 0; i < functionContext.numArgs - 6; ++i) {
        out << functionContext.vars[i + 6].to_string() << " <- stack-arg "
            << (8 * (functionContext.numArgs - 7 - i)) << "\n";
      }
    }

    for (auto& context : functionContext.partitions) {
      std::visit(
          [&out, &returnInstructionCounter](auto&& context) {
            using T = std::decay_t<decltype(context)>;

            // helper: print Operand (Var/Number) to out
            auto emit_operand = [&out](const Operand& op) {
              std::visit([&out](auto&& x) { out << x.to_string(); }, op);
            };

            // helper: print Callee (Var/Label/EnumCallee) to out
            auto emit_callee = [&out](const Callee& callee,
                                      bool isFunctionName) {
              std::visit(
                  [&out, &isFunctionName](auto&& x) {
                    using V = std::decay_t<decltype(x)>;
                    if constexpr (std::is_same_v<V, EnumCallee>) {
                      switch (x) {
                        case EnumCallee::PRINT:
                          out << "print";
                          break;
                        case EnumCallee::ALLOCATE:
                          out << "allocate";
                          break;
                        case EnumCallee::INPUT:
                          out << "input";
                          break;
                        case EnumCallee::TUPLE_ERROR:
                          out << "tuple-error";
                          break;
                        case EnumCallee::TENSOR_ERROR:
                          out << "tensor-error";
                          break;
                        default:
                          throw std::runtime_error("Invalid EnumCallee");
                      }
                    } else {
                      if (!isFunctionName) {
                        out << ":" << x.to_string().substr(1);
                      } else {
                        out << x.to_string();
                      }
                    }
                  },
                  callee);
            };

            // helper: move args into regs/stack
            auto emit_call_setup =
                [&out, &emit_operand](
                    const std::vector<Operand>& args,
                    const std::vector<std::string>& argRegisters) {
                  int numArgs = (int)args.size();
                  int regArgs = std::min(numArgs, (int)argRegisters.size());

                  for (int i = 0; i < regArgs; ++i) {
                    out << argRegisters[i] << " <- ";
                    emit_operand(args[i]);
                    out << "\n";
                  }

                  if (numArgs > (int)argRegisters.size()) {
                    for (int i = 0; i < numArgs - (int)argRegisters.size();
                         ++i) {
                      out << "mem rsp -" << (8 * (i + 2)) << " <- ";
                      emit_operand(args[i + (int)argRegisters.size()]);
                      out << "\n";
                    }
                  }
                };

            if constexpr (std::is_same_v<T, Context>) {
              for (const auto& code : context.L2Code) {
                out << code << "\n";
              }
              return;
            }

            if constexpr (std::is_same_v<T, LabelInstruction>) {
              out << context.to_string() << "\n";
              return;
            }

            if constexpr (std::is_same_v<T, CallInstruction>) {
              Callee callee = context.get_callee();
              std::vector<Operand> args = context.get_args();
              std::vector<std::string> argRegisters = {"rdi", "rsi", "rdx",
                                                       "rcx", "r8",  "r9"};

              if (std::holds_alternative<EnumCallee>(callee)) {
                EnumCallee enumCallee = std::get<EnumCallee>(callee);

                switch (enumCallee) {
                  case EnumCallee::PRINT: {
                    out << "rdi <- ";
                    emit_operand(args[0]);
                    out << "\n";
                    out << "call print 1\n";
                    break;
                  }
                  case EnumCallee::ALLOCATE: {
                    out << "rdi <- ";
                    emit_operand(args[0]);
                    out << "\n";
                    out << "rsi <- ";
                    emit_operand(args[1]);
                    out << "\n";
                    out << "call allocate 2\n";
                    break;
                  }
                  case EnumCallee::TUPLE_ERROR: {
                    out << "rdi <- ";
                    emit_operand(args[0]);
                    out << "\n";
                    out << "rsi <- ";
                    emit_operand(args[1]);
                    out << "\n";
                    out << "rdx <- ";
                    emit_operand(args[2]);
                    out << "\n";
                    out << "call tuple-error 3\n";
                    break;
                  }
                  case EnumCallee::TENSOR_ERROR: {
                    emit_call_setup(args, argRegisters);
                    out << "call tensor-error " << args.size() << "\n";
                    break;
                  }
                  case EnumCallee::INPUT: {
                    out << "call input 0\n";
                    break;
                  }
                  default:
                    throw std::runtime_error("Invalid EnumCallee");
                }
              } else {
                emit_call_setup(args, argRegisters);

                out << "mem rsp -8 <- ";
                emit_callee(callee, false);
                out << "_ret" << returnInstructionCounter << "\n";

                out << "call ";
                emit_callee(callee, true);
                out << " " << args.size() << "\n";

                emit_callee(callee, false);
                out << "_ret" << returnInstructionCounter << "\n";
                ++returnInstructionCounter;
              }
              return;
            }

            if constexpr (std::is_same_v<T, AssignCallInstruction>) {
              Var dst = context.get_dst();
              Callee callee = context.get_callee();
              std::vector<Operand> args = context.get_args();
              std::vector<std::string> argRegisters = {"rdi", "rsi", "rdx",
                                                       "rcx", "r8",  "r9"};

              if (std::holds_alternative<EnumCallee>(callee)) {
                EnumCallee enumCallee = std::get<EnumCallee>(callee);

                switch (enumCallee) {
                  case EnumCallee::PRINT: {
                    out << "rdi <- ";
                    emit_operand(args[0]);
                    out << "\n";
                    out << "call print 1\n";
                    break;
                  }
                  case EnumCallee::ALLOCATE: {
                    out << "rdi <- ";
                    emit_operand(args[0]);
                    out << "\n";
                    out << "rsi <- ";
                    emit_operand(args[1]);
                    out << "\n";
                    out << "call allocate 2\n";
                    break;
                  }
                  case EnumCallee::TUPLE_ERROR: {
                    out << "rdi <- ";
                    emit_operand(args[0]);
                    out << "\n";
                    out << "rsi <- ";
                    emit_operand(args[1]);
                    out << "\n";
                    out << "rdx <- ";
                    emit_operand(args[2]);
                    out << "\n";
                    out << "call tuple-error 3\n";
                    break;
                  }
                  case EnumCallee::TENSOR_ERROR: {
                    emit_call_setup(args, argRegisters);
                    out << "call tensor-error " << args.size() << "\n";
                    break;
                  }
                  case EnumCallee::INPUT: {
                    out << "call input 0\n";
                    break;
                  }
                  default:
                    throw std::runtime_error("Invalid EnumCallee");
                }
              } else {
                emit_call_setup(args, argRegisters);

                out << "mem rsp -8 <- ";
                emit_callee(callee, false);
                out << "_ret" << returnInstructionCounter << "\n";

                out << "call ";
                emit_callee(callee, true);
                out << " " << args.size() << "\n";

                emit_callee(callee, false);
                out << "_ret" << returnInstructionCounter << "\n";
                ++returnInstructionCounter;
              }

              out << dst.to_string() << " <- rax\n";
              return;
            }

            throw std::runtime_error("Invalid variant for partition");
          },
          context);
    }

    out << ")\n";
  }

  out << ")";
  return out.str();
}

}  // namespace L3
