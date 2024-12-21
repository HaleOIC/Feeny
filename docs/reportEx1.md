# Exercise 1

## Question 1

Feeny uses `null` for false and `non-null` for true, which simplifies the representation of boolean value, we no longer need to import non-integer type into the system. By using existing value instead of dedicated boolean type, Feeny reduces memory overhead and satisfies the runtime implementation. This kind of chioce is also logically consistent, as `null` representing "nothing" or "false" is intuitive, while any non-null value representing "something" or "true" makes logical sense.

However, this approach does come with some tradeoffs, particularly in terms of readability and deviation from modern programming conventions. It may be less explicit than traditional true/false values and could be initially confusing for beginners.

## Question 2

In the file `cplx.feeny`, function `cplx` returns an Object inheriting from `Null` Object, in the body of this object, containing two attributes and five methods that can be invoked by using `.`(dot). Function `add`, `sub`, `mul`, `div`, `print` are basic operation for a complex number.

For Java version code having the same meaning:

```Java
public class Complex {
    private double real;
    private double imag;
    
    public Complex(double real, double imag) {
        this.real = real;
        this.imag = imag;
    }
    
    public Complex add(Complex c) {
        return new Complex(
            this.real + c.real,
            this.imag + c.imag
        );
    }
    
    public Complex sub(Complex c) {
        return new Complex(
            this.real - c.real,
            this.imag - c.imag
        );
    }
    
    public Complex mul(Complex c) {
        return new Complex(
            this.real * c.real - this.imag * c.imag,
            this.real * c.imag + this.imag * c.real
        );
    }
    
    public Complex div(Complex c) {
        double d = c.real * c.real + c.imag * c.imag;
        return new Complex(
            (this.real * c.real + this.imag * c.imag) / d,
            (this.imag * c.real - this.real * c.imag) / d
        );
    }
    
    @Override
    public String toString() {
        if (imag < 0) {
            return String.format("%.2f - %.2fi", real, -imag);
        } else if (imag == 0) {
            return String.format("%.2f", real);
        } else {
            return String.format("%.2f + %.2fi", real, imag);
        }
    }
```

## Question 3

In Feeny's method call semantics, when a method is invoked, a new scope is created for the method execution, and the parameter bindings are added to this new scope. The special variable this is also bound to the receiver object in this scope. This scope is then used to resolve any variable references within the method. Without the explicit `this.` prefix, any reference to `real` or `imag` would first look for these names in the method's local scope, rather than looking for them as fields of the object.

The necessity of using `this.` becomes clear when we consider that method parameters or local variables could potentially have the same names as object fields. Without the explicit `this.` prefix, a reference to `real` or `imag` would resolve to a local variable if one exists with that name, rather than accessing the object's field. This is different from some other languages like Java where unqualified names can implicitly refer to object fields. In Feeny, the explicit `this.` notation ensures unambiguous access to object fields and makes the code's behavior more predictable by clearly distinguishing between local variable access and object field access.

## Question 4

Feeny's implementation of complex numbers offers several advantages over Java through its more concise and flexible object system. Unlike Java's rigid class-based structure requiring explicit type declarations, access modifiers, and constructor definitions, Feeny uses a direct object literal syntax that allows methods and variables to be defined inline with minimal boilerplate code. The implementation can be modified or extended at runtime, and new methods can be added dynamically, which isn't possible in Java's static class system. Additionally, Feeny's approach of using object literals makes the code more readable and maintainable, as the entire implementation is contained in a single, cohesive structure without the need for separate class declarations, getter/setter methods, or explicit type annotations that are mandatory in Java.

## Question 5

The inheritance.feeny example demonstrates Feeny's object inheritance mechanism, where child objects can inherit and call methods defined in their parent object while also providing their own specialized implementations. In this code, both adder and multiplier inherit from the base() object, retaining the do-op method but implementing their own unique op method. When a.do-op(11, 7) or m.do-op(11, 7) is called, the do-op method from the base object is executed, which in turn calls the specific op method of each child object through this.op(x, y). This showcases how Feeny's object model allows for dynamic method inheritance, where a parent method can invoke a child-specific method, enabling flexible runtime polymorphism and code reuse that differs from more rigid inheritance models in languages like Java.

## Question 6

Unlike Java's statically typed system with strict initialization rules, Feeny allows variables to exist in an uninitialized or undefined state without requiring mandatory default values. This approach aligns with functional and prototype-based language paradigms, where variables are more fluid and can be created, modified, or left undefined without strict compile-time constraints. The language likely assumes that programmers will explicitly initialize variables when needed, treating undefined/uninitialized state as a valid and intentional programming construct, rather than mandating default values that would add unnecessary complexity to the language's type system and runtime behavior.

## Question 7

When porting a closure-heavy program to Feeny, you would leverage Feeny's object system as a flexible alternative to traditional closure mechanisms. In Feeny, closures can be effectively translated to objects with methods that capture and manipulate state, essentially treating functions as first-class objects with enclosed environments.

The key translation strategy would involve:

1. Converting closure-captured variables into object fields
2. Transforming closure functions into object methods
3. Using Feeny's prototype-based inheritance to maintain lexical scoping behaviors
4. Utilizing object literals to create compact, function-like constructs

For example, a JavaScript closure like:

```javaScript
function counter() {
  let count = 0;
  return function() { 
    return ++count; 
  };
}
```

Might be translated to Feeny as:

```text
defn counter () :
  object :
    var count = 0
    method increment () :
      count = count + 1
      count
```

This approach demonstrates how Feeny's object model can seamlessly replace closure-centric patterns by providing a more flexible, dynamic way of encapsulating state and behavior. The object becomes the fundamental construct, while maintaining the essential characteristics of closure-like state preservation and lexical scoping.
