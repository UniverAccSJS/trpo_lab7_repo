#include <iostream>
#include <string>
#include <cmath>
#include <cassert>

using namespace std;

// Из методички

struct Transformer;
struct Number;
struct BinaryOperation;
struct FunctionCall;
struct Variable;

struct Expression {
    virtual ~Expression() { }
    virtual double evaluate() const = 0;
    virtual Expression *transform(Transformer *tr) const = 0;
};

struct Transformer {
    virtual ~Transformer() { }
    virtual Expression *transformNumber(Number const *) = 0;
    virtual Expression *transformBinaryOperation(BinaryOperation const *) = 0;
    virtual Expression *transformFunctionCall(FunctionCall const *) = 0;
    virtual Expression *transformVariable(Variable const *) = 0;
};

struct Number : Expression {
    Number(double value) : value_(value) {}
    double value() const { return value_; }
    double evaluate() const override { return value_; }
    Expression *transform(Transformer *tr) const override {
        return tr->transformNumber(this);
    }
private:
    double value_;
};

struct BinaryOperation : Expression {
    enum { PLUS = '+', MINUS = '-', DIV = '/', MUL = '*' };

    BinaryOperation(Expression const *left, int op, Expression const *right)
        : left_(left), op_(op), right_(right) { assert(left_ && right_); }

    ~BinaryOperation() {
        delete left_;
        delete right_;
    }

    Expression const *left() const { return left_; }
    Expression const *right() const { return right_; }
    int operation() const { return op_; }

    double evaluate() const override {
        double left = left_->evaluate();
        double right = right_->evaluate();
        switch (op_) {
            case PLUS:  return left + right;
            case MINUS: return left - right;
            case DIV:   return left / right;
            case MUL:   return left * right;
            default:    return 0.0;
        }
    }

    Expression *transform(Transformer *tr) const override {
        return tr->transformBinaryOperation(this);
    }

private:
    Expression const *left_;
    Expression const *right_;
    int op_;
};

struct FunctionCall : Expression {
    FunctionCall(string const &name, Expression const *arg)
        : name_(name), arg_(arg) {
        assert(arg_);
        assert(name_ == "sqrt" || name_ == "abs");
    }

    ~FunctionCall() { delete arg_; }

    string const &name() const { return name_; }
    Expression const *arg() const { return arg_; }

    double evaluate() const override {
        if (name_ == "sqrt") return sqrt(arg_->evaluate());
        else return fabs(arg_->evaluate());
    }

    Expression *transform(Transformer *tr) const override {
        return tr->transformFunctionCall(this);
    }

private:
    string const name_;
    Expression const *arg_;
};

struct Variable : Expression {
    Variable(string const &name) : name_(name) {}
    string const &name() const { return name_; }
    double evaluate() const override { return 0.0; }
    Expression *transform(Transformer *tr) const override {
        return tr->transformVariable(this);
    }
private:
    string const name_;
};

// №1. Реализация класса CopySyntaxTree (Паттерн Visitor)
struct CopySyntaxTree : Transformer {
    Expression *transformNumber(Number const *number) override { 
        return new Number(number->value());
    }

    Expression *transformBinaryOperation(BinaryOperation const *binop) override { 
        Expression* leftCopy = binop->left()->transform(this);
        Expression* rightCopy = binop->right()->transform(this);
        return new BinaryOperation(leftCopy, binop->operation(), rightCopy);
    }

    Expression *transformFunctionCall(FunctionCall const *fcall) override {
        Expression* argCopy = fcall->arg()->transform(this);
        return new FunctionCall(fcall->name(), argCopy);
    }

    Expression *transformVariable(Variable const *var) override { 
        return new Variable(var->name());
    }
};

//2. Реализация класса FoldConstants (Сворачивание констант)
struct FoldConstants : Transformer {
    Expression *transformNumber(Number const *number) override {
        return new Number(number->value());
    }

    Expression *transformBinaryOperation(BinaryOperation const *binop) override {
        Expression* leftFolded = binop->left()->transform(this);
        Expression* rightFolded = binop->right()->transform(this);

        Number* leftNum = dynamic_cast<Number*>(leftFolded);
        Number* rightNum = dynamic_cast<Number*>(rightFolded);

        if (leftNum && rightNum) {
            double result = 0.0;
            switch (binop->operation()) {
                case BinaryOperation::PLUS:  result = leftNum->value() + rightNum->value(); break;
                case BinaryOperation::MINUS: result = leftNum->value() - rightNum->value(); break;
                case BinaryOperation::MUL:   result = leftNum->value() * rightNum->value(); break;
                case BinaryOperation::DIV:   result = leftNum->value() / rightNum->value(); break;
            }
            delete leftFolded;
            delete rightFolded;
            return new Number(result);
        }
        return new BinaryOperation(leftFolded, binop->operation(), rightFolded);
    }

    Expression *transformFunctionCall(FunctionCall const *fcall) override {
        Expression* argFolded = fcall->arg()->transform(this);
        Number* argNum = dynamic_cast<Number*>(argFolded);

        if (argNum) {
            double result = 0.0;
            if (fcall->name() == "sqrt") {
                result = sqrt(argNum->value());
            } else if (fcall->name() == "abs") {
                result = fabs(argNum->value());
            }
            delete argFolded;
            return new Number(result);
        }
        return new FunctionCall(fcall->name(), argFolded);
    }

    Expression *transformVariable(Variable const *var) override {
        return new Variable(var->name());
    }
};

//3. Вариант 4. Адаптер Объекта (PointPolar к PointDecart)
// Декартовы координаты
class PointDecart {
protected:
    double x_;
    double y_;

public:
    PointDecart(double x = 0.0, double y = 0.0) : x_(x), y_(y) {}
    virtual ~PointDecart() {}

    virtual double getX() const { return x_; }
    virtual double getY() const { return y_; }
    virtual void setX(double x) { x_ = x; }
    virtual void setY(double y) { y_ = y; }

    virtual void print() const {
        cout << "Decart Point: (" << x_ << ", " << y_ << ")" << "\n";
    }
};

// Полярные координаты (Адаптируемый класс)
class PointPolar {
private:
    double r_;     
    double phi_;   

public:
    PointPolar(double r = 0.0, double phi = 0.0) : r_(r), phi_(phi) {}

    double getR() const { return r_; }
    double getPhi() const { return phi_; }
    void setR(double r) { r_ = r; }
    void setPhi(double phi) { phi_ = phi; }

    void printPolar() const {
        double degrees = phi_ * 180.0 / M_PI;
        cout << "Polar Point: (r = " << r_ << ", phi = " << degrees << "°)" << "\n";
    }
};

// Адаптер объекта
class PointPolarAdapter : public PointDecart {
private:
    const PointPolar* polarPoint_; 

public:
    PointPolarAdapter(const PointPolar* polarPoint) : polarPoint_(polarPoint) {}

    double getX() const override {
        return polarPoint_->getR() * cos(polarPoint_->getPhi());
    }

    double getY() const override {
        return polarPoint_->getR() * sin(polarPoint_->getPhi());
    }

    void setX(double x) override {
        cout << "[Adapter Warning] Direct setX is not supported for Polar Adapter." << "\n";
    }

    void setY(double y) override {
        cout << "[Adapter Warning] Direct setY is not supported for Polar Adapter." << "\n";
    }

    void print() const override {
        cout << "Adapted Decart View: (" << getX() << ", " << getY() << ")" << "\n";
    }
};

// ============================================================================
// Демонстрация работы программы
// ============================================================================

int main() {
    // Тесты №1
    cout << "=== ТЕСТИРОВАНИЕ ЗАДАНИЯ 1 (CopySyntaxTree) ===" << "\n";
    
    // Тест 1.1: Сложное выражение из методички: abs(var * sqrt(32.0 - 16.0))
    Number* n32 = new Number(32.0);
    Number* n16 = new Number(16.0);
    BinaryOperation* minus = new BinaryOperation(n32, BinaryOperation::MINUS, n16);
    FunctionCall* callSqrt = new FunctionCall("sqrt", minus);
    Variable* var = new Variable("var");
    BinaryOperation* mult = new BinaryOperation(var, BinaryOperation::MUL, callSqrt);
    FunctionCall* callAbs = new FunctionCall("abs", mult);

    CopySyntaxTree CST;
    Expression* copiedExpr = callAbs->transform(&CST);
    
    cout << "[Тест 1.1] Оригинал (var=0): " << callAbs->evaluate() << "\n";
    cout << "[Тест 1.1] Копия    (var=0): " << copiedExpr->evaluate() << "\n";

    // Тест 1.2: Простое одиночное число
    Expression* singleNumber = new Number(42.0);
    Expression* copiedNumber = singleNumber->transform(&CST);
    cout << "[Тест 1.2] Одиночное число (оригинал): " << singleNumber->evaluate() << "\n";
    cout << "[Тест 1.2] Одиночное число (копия):    " << copiedNumber->evaluate() << "\n";

    // Тест 1.3: Выражение только из констант: (10.5 + 4.5) * 2.0
    Expression* constExpr = new BinaryOperation(
        new BinaryOperation(new Number(10.5), BinaryOperation::PLUS, new Number(4.5)),
        BinaryOperation::MUL,
        new Number(2.0)
    );
    Expression* copiedConstExpr = constExpr->transform(&CST);
    cout << "[Тест 1.3] Константное выражение (оригинал): " << constExpr->evaluate() << "\n";
    cout << "[Тест 1.3] Константное выражение (копия):    " << copiedConstExpr->evaluate() << "\n";


    // Тесты №2
    cout << "\n=== ТЕСТИРОВАНИЕ ЧАСТИ 2 (FoldConstants) ===" << "\n";
    FoldConstants FC;

    // Тест 2.1: Сворачивание дерева abs(var * sqrt(32.0 - 16.0))
    // Внутри var останется переменной, но ветка sqrt(32-16) должна свернуться в 4.0
    Expression* foldedExpr = callAbs->transform(&FC);
    cout << "[Тест 2.1] Свернутое дерево из методички (var=0): " << foldedExpr->evaluate() << "\n";

    // Тест 2.2: Полное сворачивание до одного узла (нет переменных)
    // Выражение: abs(sqrt(25.0) * -3.0) -> abs(5.0 * -3.0) -> abs(-15.0) -> 15.0
    Expression* fullFoldExpr = new FunctionCall("abs", 
        new BinaryOperation(
            new FunctionCall("sqrt", new Number(25.0)),
            BinaryOperation::MUL,
            new Number(-3.0)
        )
    );
    Expression* completelyFolded = fullFoldExpr->transform(&FC);
    
    // корень дерева стал обычным числом Number
    Number* isNumber = dynamic_cast<Number*>(completelyFolded);
    if (isNumber) {
        cout << "[Тест 2.2] Успех! Дерево полностью свернулось в один узел Number со значением: " 
             << isNumber->value() << "\n";
    } else {
        cout << "[Тест 2.2] Ошибка: Дерево не свернулось полностью." << "\n";
    }

    // Тест 2.3: Частичное сворачивание при делении на 0 (деление вычисляется как inf/nan, но сворачивается)
    Expression* divByZeroExpr = new BinaryOperation(new Number(5.0), BinaryOperation::DIV, new Number(0.0));
    Expression* foldedDivByZero = divByZeroExpr->transform(&FC);
    cout << "[Тест 2.3] Деление на ноль свернулось в: " << foldedDivByZero->evaluate() << "\n";


    // Тесты №3 Вариант 4 (Декартова система —> полярная система)
    cout << "\n=== ТЕСТИРОВАНИЕ ЧАСТИ 3 ВАРИАНТА 4 (Адаптер Объекта) ===" << "\n";

    // Тест 3.1: Точка в первой четверти (r=5, угол 30 градусов)
    PointPolar* polar1 = new PointPolar(5.0, M_PI / 6.0); 
    PointDecart* adapter1 = new PointPolarAdapter(polar1);
    cout << "[Тест 3.1] "; polar1->printPolar();
    cout << "[Тест 3.1] Через адаптер -> "; adapter1->print();

    // Тест 3.2: Точка на осях координат (r=3, угол 90 градусов - строго на оси Y)
    // Ожидаем: X = 0, Y = 3
    PointPolar* polar2 = new PointPolar(3.0, M_PI / 2.0);
    PointDecart* adapter2 = new PointPolarAdapter(polar2);
    cout << "[Тест 3.2] "; polar2->printPolar();
    cout << "[Тест 3.2] Через адаптер -> "; adapter2->print();

    // Тест 3.3: Проверка динамического обновления (Паттерн Адаптер объекта)
    // При изменении полярной точки декартовы координаты в адаптере должны измениться автоматически!
    cout << "[Тест 3.3] Изменяем исходную полярную точку (r=10, угол 0 градусов)..." << "\n";
    polar2->setR(10.0);
    polar2->setPhi(0.0); // Теперь точка должна лежать на оси X: X=10, Y=0
    cout << "[Тест 3.3] Проверка через старый адаптер -> "; 
    adapter2->print();


    // освобождение памяти
    delete callAbs;
    delete copiedExpr;
    delete singleNumber;
    delete copiedNumber;
    delete constExpr;
    delete copiedConstExpr;
    
    delete foldedExpr;
    delete fullFoldExpr;
    delete completelyFolded;
    delete divByZeroExpr;
    delete foldedDivByZero;

    delete polar1;
    delete adapter1;
    delete polar2;
    delete adapter2;

    return 0;
}
