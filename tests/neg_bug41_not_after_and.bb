; BUG-41: Not steht nur am Anfang eines Ausdrucks (parseExpr in compiler/parser.cpp)
Local a = 1, b = 0
If a And Not b Then Print "nie"
