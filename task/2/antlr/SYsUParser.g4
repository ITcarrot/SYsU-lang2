parser grammar SYsUParser;

options {
  tokenVocab=SYsULexer;
}

parenExpression
    :   LeftParen expression RightParen
    ;

callExpression
    :   Identifier LeftParen (assignmentExpression (Comma assignmentExpression)*)? RightParen
    ;

primaryExpression
    :   Identifier
    |   Constant
    |   parenExpression
    |   callExpression
    ;

postfixExpression
    :   primaryExpression
    |   postfixExpression LeftBracket expression RightBracket
    ;

unaryExpression
    :
    (postfixExpression
    |   unaryOperator unaryExpression
    )
    ;

unaryOperator
    :   Plus | Minus | Exclaim
    ;

multiplicativeExpression
    :   unaryExpression ((Star|Slash|Percent) unaryExpression)*
    ;

additiveExpression
    :   multiplicativeExpression ((Plus|Minus) multiplicativeExpression)*
    ;
compareExpression
    :   additiveExpression ((Greater|Greaterequal|Less|Lessequal) additiveExpression)*
    ;

equalExpression
    :   compareExpression ((Equalequal|Exclaimequal) compareExpression)*
    ;

ampampExpression
    :   equalExpression (Ampamp equalExpression)*
    ;

pipepipeExpression
    :   ampampExpression (Pipepipe ampampExpression)*
    ;

assignmentExpression
    :   pipepipeExpression
    |   unaryExpression Equal assignmentExpression
    ;

expression
    :   assignmentExpression (Comma assignmentExpression)*
    ;


declaration
    :   declarationSpecifiers initDeclaratorList? Semi
    ;

declarationSpecifiers
    :   declarationSpecifier+
    ;

declarationSpecifier
    :   typeSpecifier
    ;

initDeclaratorList
    :   initDeclarator (Comma initDeclarator)*
    ;

initDeclarator
    :   declarator functionParms? (Equal initializer)?
    ;


typeSpecifier
    :   Const? (Int|Void)
    ;

parmVarDeclaration
    :   declarationSpecifiers declarator?
    ;

functionParms
    :   LeftParen (parmVarDeclaration (Comma parmVarDeclaration)*)? RightParen
    ;

declarator
    :   directDeclarator
    ;

directDeclarator
    :   Identifier
    |   directDeclarator LeftBracket assignmentExpression? RightBracket
    ;

identifierList
    :   Identifier (Comma Identifier)*
    ;

initializer
    :   assignmentExpression
    |   LeftBrace initializerList? Comma? RightBrace
    ;

initializerList
    // :   designation? initializer (Comma designation? initializer)*
    :   initializer (Comma initializer)*
    ;

statement
    :   compoundStatement
    |   expressionStatement
    |   jumpStatement
    |   ifStatement
    |   whileStatement
    ;

compoundStatement
    :   LeftBrace blockItemList? RightBrace
    ;

blockItemList
    :   blockItem+
    ;

blockItem
    :   statement
    |   declaration
    ;

expressionStatement
    :   expression? Semi
    ;

jumpStatement
    :   ((Return expression?) | Break | Continue) Semi
    ;

ifStatement
    :   If LeftParen expression RightParen statement
        (Else statement)?
    ;

whileStatement
    :   While LeftParen expression RightParen statement
    ;

compilationUnit
    :   translationUnit? EOF
    ;

translationUnit
    :   externalDeclaration+
    ;

externalDeclaration
    :   functionDefinition
    |   declaration
    ;

functionDefinition
    : declarationSpecifiers directDeclarator functionParms compoundStatement
    ;

