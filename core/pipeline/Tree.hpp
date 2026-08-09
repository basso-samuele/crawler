#pragma once

#include <array>
#include <memory>
#include <unordered_map>
#include <string>
#include <stack>
#include <variant>
#include <iostream>

#include "Stage.hpp"
#include "Tokenizer.hpp"

namespace Crawler
{

class Node
{
private:
    std::array<std::shared_ptr<Node>, 16> p_Children;
    size_t p_NextChildPosition;
    std::weak_ptr<Node> p_Father;

    std::u32string p_Type;
    std::unordered_map<std::u32string, std::u32string> p_Attributes;
    std::u32string p_Value;

public:
    Node(std::shared_ptr<Node> father, std::u32string type);
    Node(std::shared_ptr<Node> father, std::u32string&& type, std::u32string&& value);
    void AddAttribute(std::u32string&& name, std::u32string&& value);
    void AddChild(std::shared_ptr<Node> child);
    std::u32string GetType();
    std::unordered_map<std::u32string, std::u32string> GetAttributes();
    std::u32string GetValue();
    const std::array<std::shared_ptr<Node>, 16>& GetChildren() const;
    size_t GetChildCount() const;
};

struct TreeBuilderContext {
    std::shared_ptr<Node> root;
    std::stack<std::shared_ptr<Node>> elementStack;
    std::u32string currentAttrName;
};

struct TreeInitState
{
};

struct OpenTagState
{
};

struct CloseTagState
{
};

struct GetTagAttributeState
{
};

struct GetTagAttributeValueState
{
};

using TreeBuilderState = std::variant<
    TreeInitState,
    OpenTagState,
    CloseTagState,
    GetTagAttributeState,
    GetTagAttributeValueState
>;

TreeBuilderState Transition(TreeInitState& state, TreeBuilderContext& context, Token& t);
TreeBuilderState Transition(OpenTagState& state, TreeBuilderContext& context, Token& t);
TreeBuilderState Transition(CloseTagState& state, TreeBuilderContext& context, Token& t);
TreeBuilderState Transition(GetTagAttributeState& state, TreeBuilderContext& context, Token& t);
TreeBuilderState Transition(GetTagAttributeValueState& state, TreeBuilderContext& context, Token& t);

class TreeBuilder : public Stage<Token, std::shared_ptr<Node>>
{
private:
    TreeBuilderContext p_Context;
    TreeBuilderState p_State;

public:
    TreeBuilder(Stream<Token>& in, TransactionalStream<std::shared_ptr<Node>>& out);
    void Process() override;
    void Finalize();
};

namespace Utils
{

void PrintNodeType(const std::shared_ptr<Node>& node, int depth);
void PrintNodeValue(const std::shared_ptr<Node>& node, int depth);
void PrintNodeAttributes(const std::shared_ptr<Node>& node, int depth);
void Indent(int depth);
void PrintTree(const std::shared_ptr<Node>& node, int depth = 0);

}

}