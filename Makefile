# Copyright 2026 Roman Starcevic
# SPDX-License-Identifier: Apache-2.0

NAME		:= librug.a

CXX			:= c++
CXXFLAGS	:= -Wall -Wextra -Werror -std=c++98
AR			:= ar rcs
RM			:= rm -rf

OBJDIR		:= .obj

INCLUDES	:= -I. -Iincludes -Isrcs

SRCS		:=	srcs/Rug/Rug.cpp \
				srcs/RugDiag/RugDiag.cpp \
				srcs/RugDiag/BuildError.cpp \
				srcs/RugDiag/entries.cpp \
				srcs/RugDiag/factories.cpp \
				srcs/RugDiag/getters.cpp \
				srcs/RugDiag/helpers.cpp \
				srcs/RugDiag/OCF.cpp \
				srcs/RugDiag/print.cpp \
				srcs/RugSession/RugSession.cpp \
				internal/automaton/LALRBuilder/LALRBuilder.cpp \
				internal/automaton/LALRBuilder/OCF.cpp \
				internal/automaton/LRBuilder/build.cpp \
				internal/automaton/LRBuilder/getters.cpp \
				internal/automaton/LRBuilder/OCF.cpp \
				internal/automaton/LRGraph/LRGraph.cpp \
				internal/automaton/LRGraph/LRState.cpp \
				internal/automaton/LRGraph/print.cpp \
				internal/common/BitSet/LookaheadSet.cpp \
				internal/common/BitSet/TerminalBitSet.cpp \
				internal/grammar/ABNFIR/ABNFIR/getters.cpp \
				internal/grammar/ABNFIR/ABNFIR/OCF.cpp \
				internal/grammar/ABNFIR/ABNFIR/print.cpp \
				internal/grammar/ABNFIR/ABNFIR/tools.cpp \
				internal/grammar/ABNFIR/ABNFParser/error.cpp \
				internal/grammar/ABNFIR/ABNFParser/OCF.cpp \
				internal/grammar/ABNFIR/ABNFParser/parsing.cpp \
				internal/grammar/ABNFIR/ABNFParser/parsing_utils.cpp \
				internal/grammar/ABNFIR/ABNFParser/token_navigation.cpp \
				internal/grammar/ABNFIR/ABNFValidator/OCF.cpp \
				internal/grammar/ABNFIR/ABNFValidator/tools.cpp \
				internal/grammar/ABNFIR/ABNFValidator/validator.cpp \
				internal/grammar/CFG/CFG/build.cpp \
				internal/grammar/CFG/CFG/debug.cpp \
				internal/grammar/CFG/CFG/derived_get.cpp \
				internal/grammar/CFG/CFG/derived_set.cpp \
				internal/grammar/CFG/CFG/fast_access.cpp \
				internal/grammar/CFG/CFG/metadata.cpp \
				internal/grammar/CFG/CFG/navigation.cpp \
				internal/grammar/CFG/CFG/OCF.cpp \
				internal/grammar/CFG/CFG/semantic_registry.cpp \
				internal/grammar/CFG/CFG/tools.cpp \
				internal/grammar/CFG/CFGBuilder/build.cpp \
				internal/grammar/CFG/CFGBuilder/build_null_first.cpp \
				internal/grammar/CFG/CFGBuilder/CFGBuilder.cpp \
				internal/grammar/CFG/CFGBuilder/literal.cpp \
				internal/grammar/CFG/CFGBuilder/lowering.cpp \
				internal/grammar/CFG/CFGBuilder/OCF.cpp \
				internal/grammar/CFG/CFGBuilder/repeat.cpp \
				internal/grammar/CFG/CFGBuilder/tools.cpp \
				internal/grammar/CFG/CFGBuilder/validation.cpp \
				internal/grammar/Diagnostic/Diagnostic.cpp \
				internal/grammar/Lexer/error.cpp \
				internal/grammar/Lexer/lex.cpp \
				internal/grammar/Lexer/lex_percent.cpp \
				internal/grammar/Lexer/OCF.cpp \
				internal/grammar/Lexer/print.cpp \
				internal/grammar/Lexer/tokenize.cpp \
				internal/grammar/Lexer/tools.cpp \
				internal/grammar/SourceFile/OCF.cpp \
				internal/grammar/SourceFile/tools.cpp \
				internal/runtime/Engine/Engine.cpp \
				internal/runtime/Engine/OCF.cpp \
				internal/runtime/Session/OCF.cpp \
				internal/runtime/Session/Session.cpp \
				internal/runtime/Session/utils.cpp \
				internal/semantics/SemanticBindings/SemanticBindings.cpp \
				internal/semantics/SemanticRuntime/SemanticRuntime.cpp \
				internal/table/Table/OCF.cpp \
				internal/table/Table/TableBuilder.cpp \
				internal/table/TableIR/getters.cpp \
				internal/table/TableIR/OCF.cpp \
				internal/table/TableIR/setters.cpp \
				internal/table/TableIRBuilder/OCF.cpp \
				internal/table/TableIRBuilder/TableIRBuilder.cpp \
				internal/table/TableView/getters.cpp \
				internal/table/TableView/lookup.cpp \
				internal/table/TableView/OCF.cpp \
				internal/table/TableView/validate.cpp

OBJS		:= $(SRCS:%.cpp=$(OBJDIR)/%.o)

CALC_DIR	:= examples/calculator
CALC_NAME	:= calculator

CALC_SRCS	:= \
	$(CALC_DIR)/main.cpp \
	$(CALC_DIR)/Calculator.cpp

CALC_OBJS	:= $(CALC_SRCS:%.cpp=$(OBJDIR)/%.o)

TICKET_DIR	:= examples/ticket
TICKET_NAME	:= ticket

TICKET_SRCS	:= \
	$(TICKET_DIR)/main.cpp \
	$(TICKET_DIR)/Ticket.cpp

TICKET_OBJS	:= $(TICKET_SRCS:%.cpp=$(OBJDIR)/%.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(AR) $@ $^

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

calculator: $(NAME) $(CALC_OBJS)
	$(CXX) $(CXXFLAGS) $(CALC_OBJS) $(NAME) -o $(CALC_NAME)

ticket: $(NAME) $(TICKET_OBJS)
	$(CXX) $(CXXFLAGS) $(TICKET_OBJS) $(NAME) -o $(TICKET_NAME)

clean:
	$(RM) $(OBJDIR)


fclean: clean
	$(RM) $(NAME)
	$(RM) $(CALC_NAME)
	$(RM) $(TICKET_NAME)

re: fclean all

.PHONY: all calculator ticket clean fclean re