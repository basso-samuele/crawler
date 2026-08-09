#include "Tree.hpp"

namespace Crawler
{

Node::Node(std::shared_ptr<Node> father, std::u32string type)
: p_NextChildPosition(0), p_Father(father), p_Type(type) { }

Node::Node(std::shared_ptr<Node> father, std::u32string&& type, std::u32string&& value)
: p_NextChildPosition(0), p_Father(father), p_Type(type), p_Value(std::move(value)) { }

void Node::AddAttribute(std::u32string&& name, std::u32string&& value) {
    this->p_Attributes.emplace(std::move(name), std::move(value));
}

void Node::AddChild(std::shared_ptr<Node> child) {
    if (!(this->p_NextChildPosition < this->p_Children.size())) throw "Error";
    this->p_Children[this->p_NextChildPosition] = child;
    this->p_NextChildPosition++;
}

std::u32string Node::GetType() {
    return this->p_Type;
}

std::unordered_map<std::u32string, std::u32string> Node::GetAttributes() {
    return this->p_Attributes;
}

std::u32string Node::GetValue() {
    return this->p_Value;
}

const std::array<std::shared_ptr<Node>, 16>& Node::GetChildren() const {
    return this->p_Children;
}

size_t Node::GetChildCount() const {
    return this->p_NextChildPosition;
}

TreeBuilderState Transition(TreeInitState& state, TreeBuilderContext& context, Token& t) {
    switch (t.GetType()) {
    case TokenType::OPENTAG:
        return OpenTagState{};
    case TokenType::TEXTCONTENT: {
        std::shared_ptr<Node> father = context.elementStack.top();
        std::shared_ptr<Node> node = std::make_shared<Node>(father, U"text", std::move(t.GetValue()));
        father->AddChild(node);
        return TreeInitState{};
    }
    case TokenType::CLOSETAG:
        return CloseTagState{};
    default:
        return TreeInitState{};
    }
}

TreeBuilderState Transition(OpenTagState& state, TreeBuilderContext& context, Token& t) {
    switch (t.GetType()) {
    case TokenType::TAGNAME: {
        if (!t.GetValue().compare(U"!DOCTYPE") || !t.GetValue().compare(U"meta"))
            return TreeInitState{};

        std::shared_ptr<Node> father = context.elementStack.top();
        const std::u32string& tokenValue = t.GetValue();
        std::shared_ptr<Node> node = std::make_shared<Node>(father, tokenValue);
        father->AddChild(node);
        context.elementStack.push(node);
        return GetTagAttributeState{};
    }
    default:
        throw "Error expected OPENTAG";
    }
}

TreeBuilderState Transition(CloseTagState& state, TreeBuilderContext& context, Token& t) {
    switch (t.GetType()) {
    case TokenType::TAGNAME: {
        std::shared_ptr<Node> latestOpenElement = context.elementStack.top();
        if (latestOpenElement->GetType().compare(t.GetValue())) throw "Invalid tag nesting.";
        context.elementStack.pop();
        return TreeInitState{};
    }
    default:
        throw "Error expected OPENTAG";
    }
}

TreeBuilderState Transition(GetTagAttributeState& state, TreeBuilderContext& context, Token& t) {
    switch (t.GetType()) {
    case TokenType::ATTRNAME:
        context.currentAttrName = t.GetValue();
        return GetTagAttributeValueState{};
    case TokenType::OPENTAG:
        return OpenTagState{};
    case TokenType::TEXTCONTENT: {
        std::shared_ptr<Node> father = context.elementStack.top();
        const char32_t* before = t.GetValue().data();
        std::shared_ptr<Node> node = std::make_shared<Node>(father, U"text", std::move(t.GetValue()));
        father->AddChild(node);
        return TreeInitState{};
    }
    case TokenType::SELFCLOSETAG:
        context.elementStack.pop();
        return TreeInitState{};
    default:
        throw "Unexpected tag";
    }
}

TreeBuilderState Transition(GetTagAttributeValueState& state, TreeBuilderContext& context, Token& t) {
    switch (t.GetType()) {
    case TokenType::ATTRVALUE:
        context.elementStack.top()->AddAttribute(std::move(context.currentAttrName), std::move(t.GetValue()));
        return GetTagAttributeState{};
    case TokenType::OPENTAG:
        return OpenTagState{};
    default:
        return GetTagAttributeState{};
    }
}

TreeBuilder::TreeBuilder(Stream<Token>& in, TransactionalStream<std::shared_ptr<Node>>& out)
: Stage<Token, std::shared_ptr<Node>>(in, out), p_State(TreeInitState{}) {
    this->p_Context.root = std::make_shared<Node>(nullptr, U"root");
    this->p_Context.elementStack.push(this->p_Context.root);
}

void TreeBuilder::Process() {
    for (Token t; this->p_In.Peek(&t);) {
        this->p_State = std::visit(
            [&](auto& current) {
                return Transition(current, this->p_Context, t);
            }, this->p_State);
    }

    if (this->p_In.End()) {
        this->p_Out.SetEndFlag();
    }
}

void TreeBuilder::Finalize() {
    this->p_Out.Put(std::move(this->p_Context.root));
}

void Utils::PrintNodeType(const std::shared_ptr<Node>& node, int depth) {
    std::cout << "Node type: ";
    for (char32_t c : node->GetType())
        std::cout << static_cast<char>(c);
    std::cout << std::endl;
}

void Utils::PrintNodeValue(const std::shared_ptr<Node>& node, int depth) {
    if (!node->GetValue().empty()) {
        Indent(depth);
        std::cout << "Node value: ";
        for (char32_t c : node->GetValue())
            std::cout << static_cast<char>(c);
        std::cout << std::endl;
    }
}

void Utils::PrintNodeAttributes(const std::shared_ptr<Node>& node, int depth) {
    if (!node->GetAttributes().empty()) {
        Indent(depth);
        std::cout << "Attributes: ";
        std::cout << std::endl;
        for (auto [name, value] : node->GetAttributes()) {
            Indent(depth + 1);
            for (char32_t c : name)
                std::cout << static_cast<char>(c);
            std::cout << " -> ";
            for (char32_t c : value)
                std::cout << static_cast<char>(c);
            std::cout << std::endl;
        }
    }
}

void Utils::Indent(int depth) {
    for (int i = 0; i < depth; ++i) std::cout << "\t";
}

void Utils::PrintTree(const std::shared_ptr<Node>& node, int depth)
{
    if (!node)
        return;

    Indent(depth);
    PrintNodeType(node, depth);
    PrintNodeValue(node, depth);
    PrintNodeAttributes(node, depth);

    if (node->GetChildCount()) {
        Indent(depth);
        std::cout << "Children: ";
        std::cout << std::endl;

        const auto& children = node->GetChildren();
        for (size_t i = 0; i < node->GetChildCount(); ++i) {
            PrintTree(children[i], depth + 1);
        }
    }
}

}