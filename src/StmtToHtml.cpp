#include "StmtToHtml.h"
#include "IROperator.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>

namespace Halide {
namespace Internal {

using std::string;

namespace {
template <typename T>
std::string to_string(T value) {
    std::ostringstream os ;
    os << value ;
    return os.str() ;
}

class StmtToHtml : public IRVisitor {

    static const std::string css, js;

    // This allows easier access to individual elements.
    int id_count;

private:
    std::ofstream stream;

    int unique_id() { return id_count++; }

    string open_span(string cls, string id) {
        return "<span class=\""+cls+"\" id=\""+id+"\">";
    }

    string open_span(string cls, int id = -1) {
        if (id == -1) {
            id = unique_id();
        }
        return open_span(cls, to_string(id));
    }

    string close_span() {
        return "</span>";
    }

    string open_div(string cls, int id = -1) {
        if (id == -1) {
            id = unique_id();
        }
        return "<div class=\""+cls+"\" id=\""+ to_string(id) +"\">";
    }

    string close_div() {
        return "</div>\n";
    }

    string open_line() {
        return "<p class=WrapLine>";
    }
    string close_line() {
        return "</p>";
    }

    string keyword(string k) {
        return open_span("Keyword") + k + close_span();
    }
    string type(string t) {
        return open_span("Type") + t + close_span();
    }
    string symbol(string s) {
        return open_span("Symbol") + s + close_span();
    }
    string var(string v) {
        return open_span("Variable") + v + close_span();
    }

    string open_matched(const string &cls, const string &body, int id) {
        return open_span(cls + " Matched", to_string(id) + "-open") + body + close_span();
    }
    string close_matched(const string &cls, const string &body, int id) {
        return open_span(cls + " Matched", to_string(id) + "-close") + body + close_span();
    }
    string matched(const string &cls, const string &body, int id) {
        return open_span(cls + " Matched", to_string(id) + "-" + to_string(unique_id())) + body + close_span();
    }

    string open_matched(const string &body, int id) { return open_matched("", body, id); }
    string close_matched(const string &body, int id) { return close_matched("", body, id); }
    string matched(const string &body, int id) { return matched("", body, id); }

    std::vector<int> match_ids;

    string open_matched(const string &cls, const string &c) {
        int id = unique_id();
        match_ids.push_back(id);
        return open_matched(cls, c, id);
    }

    string close_matched(const string &cls, const string &c) {
        int id = match_ids.back();
        match_ids.pop_back();
        return close_matched(cls, c, id);
    }

    string open_matched(const string &body) { return open_matched("", body); }
    string close_matched(const string &body) { return close_matched("", body); }

    void print_list(const string &l, const std::vector<Expr> &args, const string &r) {
        int id = unique_id();
        stream << open_matched(l, id);
        for (size_t i = 0; i < args.size(); i++) {
            if (i > 0) {
                stream << matched(",", id) << " ";
            }
            print(args[i]);
        }
        stream << close_matched(r, id);
    }

    string open_expand_button(int &id) {
        id = unique_id();
        std::stringstream button;
        button << "<a class=ExpandButton onclick=\"return toggle(" << id << ");\" href=_blank>"
               << "<div style=\"position:relative; width:0; height:0;\">"
               << "<div class=ShowHide style=\"display:none;\" id=" << id << "-show" << "><i class=\"fa fa-plus-square-o\"></i></div>"
               << "<div class=ShowHide id=" << id << "-hide" << "><i class=\"fa fa-minus-square-o\"></i></div>"
               << "</div>";
        return button.str();
    }

    string close_expand_button() {
        return "</a>";
    }

    void print(Expr ir) {
        ir.accept(this);
    }

    void print(Stmt ir) {
        ir.accept(this);
    }

public:
    void visit(const IntImm *op){
        stream <<  open_span("IntImm Imm") << op->value << close_span();
    }
    void visit(const FloatImm *op){
        stream <<  open_span("FloatImm Imm") << op->value << 'f' << close_span();
    }
    void visit(const StringImm *op){
        stream << open_span("StringImm");
        stream << '"';
        for (size_t i = 0; i < op->value.size(); i++) {
            unsigned char c = op->value[i];
            if (c >= ' ' && c <= '~' && c != '\\' && c != '"') {
                stream << c;
            } else {
                stream << '\\';
                switch (c) {
                case '"':
                    stream << '"';
                    break;
                case '\\':
                    stream << '\\';
                    break;
                case '\t':
                    stream << 't';
                    break;
                case '\r':
                    stream << 'r';
                    break;
                case '\n':
                    stream << 'n';
                    break;
                default:
                    string hex_digits = "0123456789ABCDEF";
                    stream << 'x' << hex_digits[c >> 4] << hex_digits[c & 0xf];
                }
            }
        }
        stream << '"';
        stream << close_span();
    }

    void visit(const Variable *op){
        stream << var(op->name);
    }

    void visit(const Cast *op){
        stream << open_span("Cast");
        stream << open_span("Type") << op->type << close_span();
        stream << open_matched("(");
        print(op->value);
        stream << close_matched(")");
        stream << close_span();
    }

    void visit_binary_op(Expr a, Expr b, const char *op) {
        stream << open_span("BinaryOp");
        int id = unique_id();
        stream << open_matched("(", id);
        print(a);
        stream << " " << matched("Operator", op, id) << " ";
        print(b);
        stream << close_matched(")", id);
        stream << close_span();
    }

    void visit(const Add *op) { visit_binary_op(op->a, op->b, "+"); }
    void visit(const Sub *op) { visit_binary_op(op->a, op->b, "-"); }
    void visit(const Mul *op) { visit_binary_op(op->a, op->b, "*"); }
    void visit(const Div *op) { visit_binary_op(op->a, op->b, "/"); }
    void visit(const Mod *op) { visit_binary_op(op->a, op->b, "%"); }
    void visit(const And *op) { visit_binary_op(op->a, op->b, "&amp;&amp;"); }
    void visit(const Or *op) { visit_binary_op(op->a, op->b, "||"); }
    void visit(const NE *op) { visit_binary_op(op->a, op->b, "!="); }
    void visit(const LT *op) { visit_binary_op(op->a, op->b, "&lt;"); }
    void visit(const LE *op) { visit_binary_op(op->a, op->b, "&lt="); }
    void visit(const GT *op) { visit_binary_op(op->a, op->b, "&gt;"); }
    void visit(const GE *op) { visit_binary_op(op->a, op->b, "&gt;="); }
    void visit(const EQ *op) { visit_binary_op(op->a, op->b, "=="); }

    void visit(const Min *op) {
        stream << open_span("Min");
        print_list(symbol("min") + "(", vec(op->a, op->b), ")");
        stream << close_span();
    }
    void visit(const Max *op) {
        stream << open_span("Max");
        print_list(symbol("max") + "(", vec(op->a, op->b), ")");
        stream << close_span();
    }
    void visit(const Not *op) {
        stream << open_span("Not");
        stream << '!';
        print(op->a);
        stream << close_span();
    }
    void visit(const Select *op) {
        stream << open_span("Select");
        print_list(symbol("select") + "(", vec(op->condition, op->true_value, op->false_value), ")");
        stream << close_span();
    }
    void visit(const Load *op) {
        stream << open_span("Load");
        stream << var(op->name);
        stream << open_matched("[");
        print(op->index);
        stream << close_matched("]");
        stream << close_span();
    }
    void visit(const Ramp *op) {
        stream << open_span("Ramp");
        print_list(symbol("ramp") + "(", vec(op->base, op->stride, Expr(op->width)), ")");
        stream << close_span();
    }
    void visit(const Broadcast *op) {
        stream << open_span("Broadcast");
        stream << open_matched(symbol("x" + to_string(op->width)) + "(");
        print(op->value);
        stream << close_matched(")");
        stream << close_span();
    }
    void visit(const Call *op) {
        stream << open_span("Call");
        if (op->call_type == Call::Intrinsic) {
            if (op->name == Call::extract_buffer_min) {
                print(op->args[0]);
                stream << ".min" << open_matched("[");
                print(op->args[1]);
                stream << close_matched("]");
                stream << close_span();
                return;
            } else if (op->name == Call::extract_buffer_max) {
                print(op->args[0]);
                stream << ".max" << open_matched("[");
                print(op->args[1]);
                stream << close_matched("]");
                stream << close_span();
                return;
            }
        }
        print_list(symbol(op->name) + "(", op->args, ")");
        stream << close_span();
    }

    void visit(const Let *op) {
        stream << open_span("Let");
        int id = unique_id();
        stream << open_matched("(" + keyword("let"), id) << " ";
        print(op->name);
        stream << " " << matched("Operator Assign", "=", id) << " ";
        print(op->value);
        stream << " " << matched("Keyword", "in", id) << " ";
        print(op->body);
        stream << close_matched(")", id) << close_span();
    }
    void visit(const LetStmt *op) {
        stream << open_div("LetStmt") << open_line();
        stream << open_matched("Keyword", "let") << " ";
        stream << var(op->name);
        stream << " " << close_matched("Operator Assign", "=") << " ";
        print(op->value);
        stream << close_line();
        op->body.accept(this);
        stream << close_div();
    }
    void visit(const AssertStmt *op) {
        stream << open_div("AssertStmt WrapLine");
        std::vector<Expr> args;
        args.push_back(op->condition);
        args.push_back(op->message);
        std::copy(op->args.begin(), op->args.end(), std::back_inserter(args));
        print_list(symbol("assert") + "(", args, ")");
        stream << close_div();
    }
    void visit(const Pipeline *op) {
        stream << open_div("Produce");
        int produce_id = 0;
        stream << open_expand_button(produce_id)
               << keyword("produce") << " " << var(op->name)
               << close_expand_button();
        stream << " " << open_matched("{");
        stream << open_div("ProduceBody Indent", produce_id);
        print(op->produce);
        stream << close_div();
        stream << close_matched("}");
        stream << close_div();
        if (op->update.defined()) {
            stream << open_div("Update");
            int update_id = 0;
            stream << open_expand_button(update_id)
                   << keyword("update") << " " << var(op->name)
                   << close_expand_button();
            stream << " " << open_matched("{");
            stream << open_div("UpdateBody Indent", update_id);
            print(op->update);
            stream << close_div();
            stream << close_matched("}");
            stream << close_div();
        }
        print(op->consume);

    }
    void visit(const For *op) {
        stream << open_div("For");

        int id = 0;
        stream << open_expand_button(id);
        if (op->for_type == 0) {
            stream << keyword("for");
        } else {
            stream << keyword("parallel");
        }
        stream << " ";
        print_list("(", vec(Variable::make(Int(32), op->name), op->min, op->extent), ")");
        stream << close_expand_button();
        stream << " " << open_matched("{");
        stream << open_div("ForBody Indent", id);
        print(op->body);
        stream << close_div();
        stream << close_matched("}");
        stream << close_div();
    }
    void visit(const Store *op) {
        stream << open_div("Store WrapLine");
        stream << var(op->name);
        stream << open_matched("[");
        print(op->index);
        stream << close_matched("]")
               << " " << open_span("Operator Assign") << "=" << close_span() << " ";
        stream << open_span("StoreValue");
        print(op->value);
        stream << close_span();
        stream << close_div();
    }
    void visit(const Provide *op) {
        stream << open_div("Provide WrapLine");
        stream << var(op->name);
        print_list("(", op->args, ")");
        stream << " = ";
        if (op->values.size() > 1) {
            print_list("{", op->values, "}");
        } else {
            print(op->values[0]);
        }
        stream << close_div();
    }
    void visit(const Allocate *op) {
        stream << open_div("Allocate");
        stream << keyword("allocate") << " ";
        stream << var(op->name);
        stream << open_matched("[");
        stream << open_span("Type") << op->type << close_span();
        for (size_t i = 0; i < op->extents.size(); i++) {
            stream  << " * ";
            print(op->extents[i]);
        }
        stream << close_matched("]");
        if (!is_one(op->condition)) {
            stream << " if ";
            print(op->condition);
        }
        stream << open_div("AllocateBody");
        print(op->body);
        stream << close_div();
        stream << close_div();
    }
    void visit(const Free *op) {
        stream << open_div("Free WrapLine");
        stream << keyword("free") << " ";
        stream << var(op->name);
        stream << close_div();
    }
    void visit(const Realize *op) {
        stream << open_div("Realize");
        int id;
        stream << open_expand_button(id);
        stream << keyword("realize") << " " << var(op->name) << open_matched("(");
        for (size_t i = 0; i < op->bounds.size(); i++) {
            print_list("[", vec(op->bounds[i].min, op->bounds[i].extent), "]");
            if (i < op->bounds.size() - 1) stream << ", ";
        }
        stream << close_matched(")");
        if (!is_one(op->condition)) {
            stream << " " << keyword("if") << " ";
            print(op->condition);
        }
        stream << close_expand_button();

        stream << " " << open_matched("{");
        stream << open_div("RealizeBody Indent", id);
        print(op->body);
        stream << close_div();
        stream << close_matched("}");
        stream << close_div();
    }
    void visit(const Block *op) {
        stream << open_div("Block");
        print(op->first);
        if (op->rest.defined()) print(op->rest);
        stream << close_div();
    }
    void visit(const IfThenElse *op) {
        stream << open_div("IfThenElse");
        stream << keyword("if") << " " << open_matched("(");
        while (1) {
            print(op->condition);
            stream << close_matched(")")
                   << " " << open_matched("{"); // close if (or else if) span
            stream << open_div("ThenBody Indent");
            print(op->then_case);
            stream << close_div(); // close thenbody div

            if (!op->else_case.defined()) {
                stream << close_matched("}");
                break;
            }

            if (const IfThenElse *nested_if = op->else_case.as<IfThenElse>()) {
                stream << close_matched("}") << " " << keyword("else if") << " " << open_matched("(");
                op = nested_if;
            } else {
                stream << close_matched("}") << " " << keyword("else") << " " << open_matched("{");
                stream << open_div("ElseBody Indent");
                print(op->else_case);
                stream << close_div();
                break;
            }
        }
        stream << close_div(); // Closing ifthenelse div.
    }

    void visit(const Evaluate *op) {
        stream << open_div("Evaluate");
        print(op->value);
        stream << close_div();
    }

    StmtToHtml(string filename){
        id_count = 0;
        stream.open(filename.c_str());
        stream << "<head>";
        stream << "<style type=\"text/css\">" << css << "</style>\n";
        stream << "<script language=\"javascript\" type=\"text/javascript\">" + js + "</script>\n";
        stream <<"<link rel=\"stylesheet\" type=\"text/css\" href=\"my.css\">\n";
        stream << "<script language=\"javascript\" type=\"text/javascript\" src=\"my.js\"></script>\n";
        stream << "<link href=\"http://maxcdn.bootstrapcdn.com/font-awesome/4.1.0/css/font-awesome.min.css\" rel=\"stylesheet\">\n";
        stream << "<script src=\"http://code.jquery.com/jquery-1.10.2.js\"></script>\n";
        stream << "</head>\n <body>\n";
    }

    void generate(Stmt s){
        print(s);
        stream << "<script>\n"
               << "$( \".Matched\" ).each( function() {\n"
               << "    this.onmouseover = function() { $(\"[id^=\" + this.id.split('-')[0] + \"-]\").addClass(\"Highlight\"); }\n"
               << "    this.onmouseout = function() { $(\"[id^=\" + this.id.split('-')[0] + \"-]\").removeClass(\"Highlight\"); }\n"
               << "} );\n"
               << "</script>\n";
        stream << "</body>";
        stream.close();
    }

    ~StmtToHtml(){}
};

const std::string StmtToHtml::css = "\n \
body { font-family: Consolas, \"Liberation Mono\", Menlo, Courier, monospace; font-size: 12px; background: #f8f8f8; } \n \
a, a:hover, a:visited, a:active { color: inherit; text-decoration: none; } \n \
p.WrapLine { margin: 0px; margin-left: 30px; text-indent:-30px; } \n \
div.WrapLine { margin-left: 30px; text-indent:-30px; } \n \
div.Indent { padding-left: 15px; }\n \
div.ShowHide { position:absolute; left:-12px; width:12px; height:12px; } \n \
span.Comment { color: #998; font-style: italic; }\n \
span.Keyword { color: #333; font-weight: bold; }\n \
span.Assign { color: #d14; font-weight: bold; }\n \
span.Symbol { color: #990073; }\n \
span.Type { color: #445588; font-weight: bold; }\n \
span.StringImm { color: #d14; }\n \
span.IntImm { color: #099; }\n \
span.FloatImm { color: #099; }\n \
span.Highlight { font-weight: bold; background-color: #FF0; }\n \
";

const std::string StmtToHtml::js = "\n \
window.onload = function () { \n \
// adding jquery \n \
var script = document.createElement('script'); \n \
script.src = 'http://code.jquery.com/jquery-2.1.1.js'; \n \
script.type = 'text/javascript'; \n \
document.getElementsByTagName('head')[0].appendChild(script); \n \
fold = function(selector) { \n \
    selector.each(function() { \n \
        $(this).attr('title', $(this).text().replace(/\"/g, \"'\")); \n \
        $(this).text(\"...\"); \n \
    }); \n \
}; \n \
unfold = function(select) { \n \
    selector.each(function() { \n \
        $(this).text($(this).attr('title')); \n \
        // $(this).attr('title', $(this).text().replace('\"',\"'\")); \n \
        // $(this).text(\"...\"); \n \
    }); \n \
}; \n \
foldClass = function(className) { fold($('.'+className)); }; \n \
unfoldClass = function(className) { unfold($('.'+className)); }; \n \
};\n \
function toggle(id) { \n \
    e = document.getElementById(id); \n \
    show = document.getElementById(id + '-show'); \n \
    hide = document.getElementById(id + '-hide'); \n \
    if (e.style.display != 'none') { \n \
        e.style.display = 'none'; \n \
        show.style.display = 'block'; \n \
        hide.style.display = 'none'; \n \
    } else { \n \
        e.style.display = 'block'; \n \
        show.style.display = 'none'; \n \
        hide.style.display = 'block'; \n \
    } \n \
    return false; \n \
}";
}

void print_to_html(string filename, Stmt s) {
    StmtToHtml sth(filename);
    sth.generate(s);
}

}
}