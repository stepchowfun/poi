#include <cassert>
#include <cstddef>
#include <cstdint>
#include <poi/optimizer.h>
#include <unordered_map>
#include <unordered_set>

namespace Poi {
  void reverse_pass_input(
    std::uint16_t input_register,
    std::unordered_set<std::uint16_t> &alive,
    std::unordered_set<std::uint16_t> &last_read
  ) {
    if (alive.insert(input_register).second) {
      last_read.insert(input_register);
    }
  }

  void reverse_pass_output(
    std::uint16_t output_register,
    std::unordered_set<std::uint16_t> &alive
  ) {
    auto iter = alive.find(output_register);
    if (iter != alive.end()) {
      alive.erase(iter);
    }
  }

  std::uint16_t forward_pass_read(
    std::uint16_t old,
    std::unordered_map<std::uint16_t, std::uint16_t> &old_to_new
  ) {
    return old_to_new.find(old)->second;
  }

  void forward_pass_elim_registers(
    std::unordered_set<std::uint16_t> &free_registers,
    std::unordered_map<std::uint16_t, std::uint16_t> &old_to_new,
    std::unordered_map<std::uint16_t, std::uint16_t> &new_refcounts,
    std::unordered_set<std::uint16_t> &registers
  ) {
    for (auto &old_reg : registers) {
      auto new_reg = forward_pass_read(old_reg, old_to_new);
      auto new_refcount = new_refcounts.find(new_reg);
      --(new_refcount->second);
      if (new_refcount->second == 0) {
        free_registers.insert(new_reg);
        new_refcounts.erase(new_refcount);
      }
      old_to_new.erase(old_reg);
    }
  }

  std::uint16_t forward_pass_write(
    std::uint16_t old,
    std::uint16_t &total_registers,
    std::unordered_set<std::uint16_t> &free_registers,
    std::unordered_map<std::uint16_t, std::uint16_t> &old_to_new,
    std::unordered_map<std::uint16_t, std::uint16_t> &new_refcounts
  ) {
    auto free_reg = free_registers.begin();
    if (free_reg == free_registers.end()) {
      auto reg = total_registers;
      old_to_new.insert({ old, reg });
      auto new_refcount = new_refcounts.find(reg);
      if (new_refcount == new_refcounts.end()) {
        new_refcounts.insert({ reg, 1 });
      } else {
        ++(new_refcount->second);
      }
      ++total_registers;
      return reg;
    } else {
      auto reg = *free_reg;
      free_registers.erase(free_reg);
      old_to_new.insert({ old, reg });
      auto new_refcount = new_refcounts.find(reg);
      if (new_refcount == new_refcounts.end()) {
        new_refcounts.insert({ reg, 1 });
      } else {
        ++(new_refcount->second);
      }
      return reg;
    }
  }

  const std::shared_ptr<const IrBlock> allocate_registers(
    const std::shared_ptr<const IrBlock> block
  ) {
    // Iterate through the instructions in reverse, marking the last time each
    // register was read.
    std::unordered_set<std::uint16_t> alive;
    std::vector<std::unordered_set<std::uint16_t>> last_read(
      block->instructions->size()
    );
    for (std::size_t i = block->instructions->size() - 1; true; --i) {
      #ifndef NDEBUG
        auto matched = false;
      #endif

      auto bc_begin_fixpoint = std::dynamic_pointer_cast<
        const IrBeginFixpoint
      >(block->instructions->at(i));
      if (bc_begin_fixpoint) {
        reverse_pass_output(bc_begin_fixpoint->destination, alive);
        #ifndef NDEBUG
          matched = true;
        #endif
      }

      auto bc_call = std::dynamic_pointer_cast<
        const IrCall
      >(block->instructions->at(i));
      if (bc_call) {
        reverse_pass_output(bc_call->destination, alive);
        reverse_pass_input(bc_call->function, alive, last_read[i]);
        reverse_pass_input(bc_call->argument, alive, last_read[i]);
        #ifndef NDEBUG
          matched = true;
        #endif
      }

      auto bc_create_function = std::dynamic_pointer_cast<
        const IrCreateFunction
      >(block->instructions->at(i));
      if (bc_create_function) {
        reverse_pass_output(bc_create_function->destination, alive);
        for (auto &capture : *(bc_create_function->captures)) {
          reverse_pass_input(capture, alive, last_read[i]);
        }
        #ifndef NDEBUG
          matched = true;
        #endif
      }

      auto bc_deref_fixpoint = std::dynamic_pointer_cast<
        const IrDerefFixpoint
      >(block->instructions->at(i));
      if (bc_deref_fixpoint) {
        reverse_pass_output(bc_deref_fixpoint->destination, alive);
        reverse_pass_input(bc_deref_fixpoint->fixpoint, alive, last_read[i]);
        #ifndef NDEBUG
          matched = true;
        #endif
      }

      auto bc_end_fixpoint = std::dynamic_pointer_cast<
        const IrEndFixpoint
      >(block->instructions->at(i));
      if (bc_end_fixpoint) {
        reverse_pass_output(bc_end_fixpoint->fixpoint, alive);
        reverse_pass_input(bc_end_fixpoint->fixpoint, alive, last_read[i]);
        reverse_pass_input(bc_end_fixpoint->target, alive, last_read[i]);
        #ifndef NDEBUG
          matched = true;
        #endif
      }

      auto bc_exit = std::dynamic_pointer_cast<
        const IrExit
      >(block->instructions->at(i));
      if (bc_exit) {
        reverse_pass_input(bc_exit->value, alive, last_read[i]);
        #ifndef NDEBUG
          matched = true;
        #endif
      }

      auto bc_move = std::dynamic_pointer_cast<
        const IrMove
      >(block->instructions->at(i));
      if (bc_move) {
        reverse_pass_output(bc_move->destination, alive);
        reverse_pass_input(bc_move->source, alive, last_read[i]);
        #ifndef NDEBUG
          matched = true;
        #endif
      }

      auto bc_return = std::dynamic_pointer_cast<
        const IrReturn
      >(block->instructions->at(i));
      if (bc_return) {
        reverse_pass_input(bc_return->value, alive, last_read[i]);
        #ifndef NDEBUG
          matched = true;
        #endif
      }

      auto bc_tail_call = std::dynamic_pointer_cast<
        const IrTailCall
      >(block->instructions->at(i));
      if (bc_tail_call) {
        reverse_pass_input(bc_tail_call->function, alive, last_read[i]);
        reverse_pass_input(bc_tail_call->argument, alive, last_read[i]);
        #ifndef NDEBUG
          matched = true;
        #endif
      }

      assert(matched);
      if (i == 0) {
        break;
      }
    }

    // Do a forward pass through the instructions to allocate the registers.
    auto new_block = std::make_shared<IrBlock>();
    std::unordered_map<std::uint16_t, std::uint16_t> old_to_new;
    std::unordered_map<std::uint16_t, std::uint16_t> new_refcounts;
    std::uint16_t total_registers = 0;
    for (auto &reg : alive) {
      old_to_new.insert({ reg, reg });
      new_refcounts.insert({ reg, 1 });
      if (total_registers <= reg) {
        total_registers = reg + 1;
      }
    }
    std::unordered_set<std::uint16_t> free_registers;
    for (std::uint16_t i = 0; i < total_registers; ++i) {
      if (alive.find(i) == alive.end()) {
        free_registers.insert(i);
      }
    }
    for (std::size_t i = 0; i < block->instructions->size(); ++i) {
      #ifndef NDEBUG
        auto matched = false;
      #endif

      auto bc_begin_fixpoint = std::dynamic_pointer_cast<
        const IrBeginFixpoint
      >(block->instructions->at(i));
      if (bc_begin_fixpoint) {
        forward_pass_elim_registers(
          free_registers,
          old_to_new,
          new_refcounts,
          last_read[i]
        );
        auto reg_destination = forward_pass_write(
          bc_begin_fixpoint->destination,
          total_registers,
          free_registers,
          old_to_new,
          new_refcounts
        );
        new_block->instructions->push_back(
          std::make_shared<IrBeginFixpoint>(
            reg_destination,
            bc_begin_fixpoint->node
          )
        );
        #ifndef NDEBUG
          matched = true;
        #endif
      }

      auto bc_call = std::dynamic_pointer_cast<
        const IrCall
      >(block->instructions->at(i));
      if (bc_call) {
        auto reg_function = forward_pass_read(bc_call->function, old_to_new);
        auto reg_argument = forward_pass_read(bc_call->argument, old_to_new);
        forward_pass_elim_registers(
          free_registers,
          old_to_new,
          new_refcounts,
          last_read[i]
        );
        auto reg_destination = forward_pass_write(
          bc_call->destination,
          total_registers,
          free_registers,
          old_to_new,
          new_refcounts
        );
        new_block->instructions->push_back(
          std::make_shared<IrCall>(
            reg_destination,
            reg_function,
            reg_argument,
            bc_call->node
          )
        );
        #ifndef NDEBUG
          matched = true;
        #endif
      }

      auto bc_create_function = std::dynamic_pointer_cast<
        const IrCreateFunction
      >(block->instructions->at(i));
      if (bc_create_function) {
        auto captures = std::make_shared<std::vector<std::uint16_t>>();
        for (auto &capture : *(bc_create_function->captures)) {
          captures->push_back(forward_pass_read(capture, old_to_new));
        }
        forward_pass_elim_registers(
          free_registers,
          old_to_new,
          new_refcounts,
          last_read[i]
        );
        auto reg_destination = forward_pass_write(
          bc_create_function->destination,
          total_registers,
          free_registers,
          old_to_new,
          new_refcounts
        );
        new_block->instructions->push_back(
          std::make_shared<IrCreateFunction>(
            reg_destination,
            optimize(bc_create_function->body),
            captures,
            bc_create_function->node
          )
        );
        #ifndef NDEBUG
          matched = true;
        #endif
      }

      auto bc_deref_fixpoint = std::dynamic_pointer_cast<
        const IrDerefFixpoint
      >(block->instructions->at(i));
      if (bc_deref_fixpoint) {
        auto reg_fixpoint = forward_pass_read(
          bc_deref_fixpoint->fixpoint,
          old_to_new
        );
        forward_pass_elim_registers(
          free_registers,
          old_to_new,
          new_refcounts,
          last_read[i]
        );
        auto reg_destination = forward_pass_write(
          bc_deref_fixpoint->destination,
          total_registers,
          free_registers,
          old_to_new,
          new_refcounts
        );
        new_block->instructions->push_back(
          std::make_shared<IrDerefFixpoint>(
            reg_destination,
            reg_fixpoint,
            bc_deref_fixpoint->node
          )
        );
        #ifndef NDEBUG
          matched = true;
        #endif
      }

      auto bc_end_fixpoint = std::dynamic_pointer_cast<
        const IrEndFixpoint
      >(block->instructions->at(i));
      if (bc_end_fixpoint) {
        auto reg_fixpoint = forward_pass_read(
          bc_end_fixpoint->fixpoint,
          old_to_new
        );
        auto reg_target = forward_pass_read(
          bc_end_fixpoint->target,
          old_to_new
        );
        new_block->instructions->push_back(
          std::make_shared<IrEndFixpoint>(
            reg_fixpoint,
            reg_target,
            bc_end_fixpoint->node
          )
        );
        #ifndef NDEBUG
          matched = true;
        #endif
      }

      auto bc_exit = std::dynamic_pointer_cast<
        const IrExit
      >(block->instructions->at(i));
      if (bc_exit) {
        auto reg_value = forward_pass_read(bc_exit->value, old_to_new);
        forward_pass_elim_registers(
          free_registers,
          old_to_new,
          new_refcounts,
          last_read[i]
        );
        new_block->instructions->push_back(
          std::make_shared<IrExit>(reg_value, bc_exit->node)
        );
        return new_block;
      }

      auto bc_move = std::dynamic_pointer_cast<
        const IrMove
      >(block->instructions->at(i));
      if (bc_move) {
        auto reg_source = forward_pass_read(bc_move->source, old_to_new);
        auto previous_new_reg = old_to_new.find(bc_move->destination);
        if (previous_new_reg != old_to_new.end()) {
          auto new_refcount = new_refcounts.find(previous_new_reg->second);
          --(new_refcount->second);
          old_to_new.erase(bc_move->destination);
        }
        old_to_new.insert({ bc_move->destination, reg_source });
        auto new_refcount = new_refcounts.find(reg_source);
        if (new_refcount == new_refcounts.end()) {
          new_refcounts.insert({ reg_source, 1 });
        } else {
          ++(new_refcount->second);
        }
        #ifndef NDEBUG
          matched = true;
        #endif
      }

      auto bc_return = std::dynamic_pointer_cast<
        const IrReturn
      >(block->instructions->at(i));
      if (bc_return) {
        auto reg_value = forward_pass_read(bc_return->value, old_to_new);
        forward_pass_elim_registers(
          free_registers,
          old_to_new,
          new_refcounts,
          last_read[i]
        );
        new_block->instructions->push_back(
          std::make_shared<IrReturn>(reg_value, bc_return->node)
        );
        return new_block;
      }

      auto bc_tail_call = std::dynamic_pointer_cast<
        const IrTailCall
      >(block->instructions->at(i));
      if (bc_tail_call) {
        auto reg_function = forward_pass_read(
          bc_tail_call->function,
          old_to_new
        );
        auto reg_argument = forward_pass_read(
          bc_tail_call->argument,
          old_to_new
        );
        forward_pass_elim_registers(
          free_registers,
          old_to_new,
          new_refcounts,
          last_read[i]
        );
        new_block->instructions->push_back(
          std::make_shared<IrTailCall>(
            reg_function,
            reg_argument,
            bc_tail_call->node
          )
        );
        return new_block;
      }

      assert(matched);
    }

    return new_block;
  }
}

const std::shared_ptr<const Poi::IrBlock> Poi::optimize(
  const std::shared_ptr<const Poi::IrBlock> block
) {
  // Perform register allocation.
  return allocate_registers(block);
}
