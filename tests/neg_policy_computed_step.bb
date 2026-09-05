; ForNode::semant verlangt einen konstanten Step:
;   if( !stepExpr->constNode() ) ex( "Step value must be constant" );
; Ein berechneter Step war eine stillschweigende Grosszuegigkeit von uns.
Local n% = 2
For i = 1 To 5 Step n
Next
