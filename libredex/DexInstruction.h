/**
 * Copyright (c) 2016-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include <assert.h>
#include <cstring>
#include <list>
#include <string>
#include <utility>

#include "Debug.h"
#include "DexDefs.h"
#include "DexOpcode.h"
#include "Gatherable.h"

#define MAX_ARG_COUNT (4)

class DexIdx;
class DexOutputIdx;

class DexInstruction : public Gatherable {
 protected:
  enum {
    REF_NONE,
    REF_STRING,
    REF_TYPE,
    REF_FIELD,
    REF_METHOD
  } m_ref_type{REF_NONE};

 private:
  uint16_t m_opcode;
  uint16_t m_arg[MAX_ARG_COUNT];

 protected:
  uint16_t m_count;

  // use clone() instead
  DexInstruction(const DexInstruction&) = default;

  // Ref-less opcodes, largest size is 5 insns.
  // If the constructor is called with a non-numeric
  // count, we'll have to add a assert here.
  // Holds formats:
  // 10x 11x 11n 12x 22x 21s 21h 31i 32x 51l
  DexInstruction(const uint16_t* opcodes, int count) : Gatherable() {
    m_opcode = *opcodes++;
    m_count = count;
    for (int i = 0; i < count; i++) {
      m_arg[i] = opcodes[i];
    }
  }

 public:
  DexInstruction(uint16_t op)
      : Gatherable(), m_opcode(op), m_count(count_from_opcode()) {}

  DexInstruction(uint16_t opcode, uint16_t arg) : DexInstruction(opcode) {
    assert(m_count == 1);
    m_arg[0] = arg;
  }

 protected:
  void encode_args(uint16_t*& insns) {
    for (int i = 0; i < m_count; i++) {
      *insns++ = m_arg[i];
    }
  }

  void encode_opcode(DexOutputIdx* dodx, uint16_t*& insns) {
    *insns++ = m_opcode;
  }

 public:
  static DexInstruction* make_instruction(DexIdx* idx, const uint16_t*& insns);
  /* Creates the right subclass of DexInstruction for the given opcode */
  static DexInstruction* make_instruction(DexOpcode);
  virtual void encode(DexOutputIdx* dodx, uint16_t*& insns);
  virtual uint16_t size() const;
  virtual DexInstruction* clone() const { return new DexInstruction(*this); }
  bool operator==(const DexInstruction&) const;

  bool has_string() const { return m_ref_type == REF_STRING; }
  bool has_type() const { return m_ref_type == REF_TYPE; }
  bool has_field() const { return m_ref_type == REF_FIELD; }
  bool has_method() const { return m_ref_type == REF_METHOD; }

  /*
   * Number of registers used.
   */
  unsigned dests_size() const;
  unsigned srcs_size() const;
  bool has_arg_word_count() const;

  /*
   * Accessors for logical parts of the instruction.
   */
  DexOpcode opcode() const;
  uint16_t dest() const;
  uint16_t src(int i) const;
  uint16_t arg_word_count() const;
  uint16_t range_base() const;
  uint16_t range_size() const;
  int64_t literal() const;
  int32_t offset() const;

  /*
   * Setters for logical parts of the instruction.
   */
  DexInstruction* set_opcode(DexOpcode);
  DexInstruction* set_dest(uint16_t vreg);
  DexInstruction* set_src(int i, uint16_t vreg);
  DexInstruction* set_srcs(const std::vector<uint16_t>& vregs);
  DexInstruction* set_arg_word_count(uint16_t count);
  DexInstruction* set_range_base(uint16_t base);
  DexInstruction* set_range_size(uint16_t size);
  DexInstruction* set_literal(int64_t literal);
  DexInstruction* set_offset(int32_t offset);

  /*
   * The number of shorts needed to encode the args.
   */
  uint16_t count() { return m_count; }

  void verify_encoding() const;

  friend std::string show(const DexInstruction* op);

 private:
  unsigned count_from_opcode() const;
};

class DexOpcodeString : public DexInstruction {
 private:
  DexString* m_string;

 public:
  virtual uint16_t size() const;
  virtual void encode(DexOutputIdx* dodx, uint16_t*& insns);
  virtual void gather_strings(std::vector<DexString*>& lstring) const;
  virtual DexOpcodeString* clone() const { return new DexOpcodeString(*this); }

  DexOpcodeString(uint16_t opcode, DexString* str) : DexInstruction(opcode) {
    m_string = str;
    m_ref_type = REF_STRING;
  }

  DexString* get_string() const { return m_string; }

  bool jumbo() const { return opcode() == OPCODE_CONST_STRING_JUMBO; }

  void set_string(DexString* str) { m_string = str; }
};

class DexOpcodeType : public DexInstruction {
 private:
  DexType* m_type;

 public:
  virtual uint16_t size() const;
  virtual void encode(DexOutputIdx* dodx, uint16_t*& insns);
  virtual void gather_types(std::vector<DexType*>& ltype) const;
  virtual DexOpcodeType* clone() const { return new DexOpcodeType(*this); }

  DexOpcodeType(uint16_t opcode, DexType* type) : DexInstruction(opcode) {
    m_type = type;
    m_ref_type = REF_TYPE;
  }

  DexOpcodeType(uint16_t opcode, DexType* type, uint16_t arg)
      : DexInstruction(opcode, arg) {
    m_type = type;
    m_ref_type = REF_TYPE;
  }

  DexType* get_type() const { return m_type; }

  void set_type(DexType* type) { m_type = type; }
};

class DexOpcodeField : public DexInstruction {
 private:
  DexField* m_field;

 public:
  virtual uint16_t size() const;
  virtual void encode(DexOutputIdx* dodx, uint16_t*& insns);
  virtual void gather_fields(std::vector<DexField*>& lfield) const;
  virtual DexOpcodeField* clone() const { return new DexOpcodeField(*this); }

  DexOpcodeField(uint16_t opcode, DexField* field) : DexInstruction(opcode) {
    m_field = field;
    m_ref_type = REF_FIELD;
  }

  DexField* get_field() const { return m_field; }
  void set_field(DexField* field) { m_field = field; }
};

class DexOpcodeMethod : public DexInstruction {
 private:
  DexMethod* m_method;

 public:
  virtual uint16_t size() const;
  virtual void encode(DexOutputIdx* dodx, uint16_t*& insns);
  virtual void gather_methods(std::vector<DexMethod*>& lmethod) const;
  virtual DexOpcodeMethod* clone() const { return new DexOpcodeMethod(*this); }

  DexOpcodeMethod(uint16_t opcode, DexMethod* meth, uint16_t arg = 0)
      : DexInstruction(opcode, arg) {
    m_method = meth;
    m_ref_type = REF_METHOD;
  }

  DexMethod* get_method() const { return m_method; }

  void set_method(DexMethod* method) { m_method = method; }
};

class DexOpcodeData : public DexInstruction {
 private:
  uint16_t m_data_count;
  uint16_t* m_data;

 public:
  virtual uint16_t size() const;
  virtual void encode(DexOutputIdx* dodx, uint16_t*& insns);
  virtual DexOpcodeData* clone() const { return new DexOpcodeData(*this); }

  DexOpcodeData(const uint16_t* opcodes, int count)
      : DexInstruction(opcodes, 0),
        m_data_count(count),
        m_data(new uint16_t[count]) {
    opcodes++;
    memcpy(m_data, opcodes, count * sizeof(uint16_t));
  }

  DexOpcodeData(const DexOpcodeData& op)
      : DexInstruction(op),
        m_data_count(op.m_data_count),
        m_data(new uint16_t[m_data_count]) {
    memcpy(m_data, op.m_data, m_data_count * sizeof(uint16_t));
  }

  DexOpcodeData& operator=(DexOpcodeData op) {
    DexInstruction::operator=(op);
    std::swap(m_data, op.m_data);
    return *this;
  }

  ~DexOpcodeData() { delete[] m_data; }

  const uint16_t* data() { return m_data; }
};

/**
 * Return a copy of the instruction passed in.
 */
DexInstruction* copy_insn(DexInstruction* insn);

////////////////////////////////////////////////////////////////////////////////
// Convenient predicates for opcode classes.

inline bool is_iget(DexOpcode op) {
  return op >= OPCODE_IGET && op <= OPCODE_IGET_SHORT;
}

inline bool is_iput(DexOpcode op) {
  return op >= OPCODE_IPUT && op <= OPCODE_IPUT_SHORT;
}

inline bool is_ifield_op(DexOpcode op) {
  return op >= OPCODE_IGET && op <= OPCODE_IPUT_SHORT;
}

inline bool is_sget(DexOpcode op) {
  return op >= OPCODE_SGET && op <= OPCODE_SGET_SHORT;
}

inline bool is_sput(DexOpcode op) {
  return op >= OPCODE_SPUT && op <= OPCODE_SPUT_SHORT;
}

inline bool is_sfield_op(DexOpcode op) {
  return op >= OPCODE_SGET && op <= OPCODE_SPUT_SHORT;
}

inline bool is_move(DexOpcode op) {
  return op >= OPCODE_MOVE && op <= OPCODE_MOVE_OBJECT_16;
}

inline bool is_return(DexOpcode op) {
  return op >= OPCODE_RETURN_VOID && op <= OPCODE_RETURN_OBJECT;
}

inline bool is_return_value(DexOpcode op) {
  // OPCODE_RETURN_VOID is deliberately excluded because void isn't a "value".
  return op >= OPCODE_RETURN && op <= OPCODE_RETURN_OBJECT;
}

inline bool is_move_result(DexOpcode op) {
  return op >= OPCODE_MOVE_RESULT && op <= OPCODE_MOVE_RESULT_OBJECT;
}

inline bool is_invoke(DexOpcode op) {
  return op >= OPCODE_INVOKE_VIRTUAL && op <= OPCODE_INVOKE_INTERFACE_RANGE;
}

inline bool is_invoke_range(DexOpcode op) {
  return op >= OPCODE_INVOKE_VIRTUAL_RANGE &&
      op <= OPCODE_INVOKE_INTERFACE_RANGE;
}

inline bool is_filled_new_array(DexOpcode op) {
  return op == OPCODE_FILLED_NEW_ARRAY || op == OPCODE_FILLED_NEW_ARRAY_RANGE;
}

inline bool writes_result_register(DexOpcode op) {
  return is_invoke(op) || is_filled_new_array(op);
}

inline bool is_branch(DexOpcode op) {
  switch (op) {
  case OPCODE_PACKED_SWITCH:
  case OPCODE_SPARSE_SWITCH:
  case OPCODE_GOTO_32:
  case OPCODE_IF_EQ:
  case OPCODE_IF_NE:
  case OPCODE_IF_LT:
  case OPCODE_IF_GE:
  case OPCODE_IF_GT:
  case OPCODE_IF_LE:
  case OPCODE_IF_EQZ:
  case OPCODE_IF_NEZ:
  case OPCODE_IF_LTZ:
  case OPCODE_IF_GEZ:
  case OPCODE_IF_GTZ:
  case OPCODE_IF_LEZ:
  case OPCODE_GOTO_16:
  case OPCODE_GOTO:
    return true;
  default:
    return false;
  }
}

inline bool is_goto(DexOpcode op) {
  switch (op) {
  case OPCODE_GOTO_32:
  case OPCODE_GOTO_16:
  case OPCODE_GOTO:
    return true;
  default:
    return false;
  }
}

inline bool is_conditional_branch(DexOpcode op) {
  switch (op) {
  case OPCODE_IF_EQ:
  case OPCODE_IF_NE:
  case OPCODE_IF_LT:
  case OPCODE_IF_GE:
  case OPCODE_IF_GT:
  case OPCODE_IF_LE:
  case OPCODE_IF_EQZ:
  case OPCODE_IF_NEZ:
  case OPCODE_IF_LTZ:
  case OPCODE_IF_GEZ:
  case OPCODE_IF_GTZ:
  case OPCODE_IF_LEZ:
    return true;
  default:
    return false;
  }
}

inline bool is_multi_branch(DexOpcode op) {
  return op == OPCODE_PACKED_SWITCH || op == OPCODE_SPARSE_SWITCH;
}

inline bool is_const(DexOpcode op) {
  return op >= OPCODE_CONST_4 && op <= OPCODE_CONST_CLASS;
}

inline bool is_fopcode(DexOpcode op) {
  return op == FOPCODE_PACKED_SWITCH || op == FOPCODE_SPARSE_SWITCH ||
         op == FOPCODE_FILLED_ARRAY;
}
