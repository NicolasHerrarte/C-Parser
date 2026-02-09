# LR(1) Parser & Grammar Processor
It is still in testing phase so I would not use it for production

**LR(1) Parser Generator and Syntax Analyzer** written in C. This system automates the transition from formal grammar specifications to concrete syntax trees, utilizing advanced lookahead propagation and state-machine generation.

## Key Features

* **LR(1) Canonical Collection Engine**: Implements full closure and transition logic to generate states, expertly handling complex lookahead propagation for deterministic parsing.
* **Automated Grammar Construction**: Dynamically builds Context-Free Grammars (CFG) from specification files via an integrated scanner and regex-based lexer.
* **FIRST Set Generation**: Utilizes a fixed-point iteration algorithm to compute FIRST sets, a critical component for predictive parsing and table accuracy.
* **Conflict Detection**: Proactively identifies and reports **Shift/Reduce** and **Reduce/Reduce** conflicts during the action table generation phase.
* **Dynamic Parse Tree Synthesis**: Generates a Concrete Syntax Tree (CST) in real-time during the shift-reduce process using a flexible N-ary tree structure.
* **Efficient Stack Management**: Employs a synchronized multi-parameter stack (Tokens, States, and Tree Nodes) to manage structural reductions alongside state transitions.
* **Symbol Mapping System**: Decouples human-readable grammar symbols from internal integer IDs using an integrated dictionary for O(1) lookups.

---

## Architecture Overview

Utilizes LR(1) parsing which is designed for compiler front-ends for minimal ambiguity, this implementation takes production rules and source files as input to produce a hierarchical Parse Tree.

### Technical Specifications
| Component | Implementation Detail |
| :--- | :--- |
| **Language** | C (Optimized for memory and speed) |
| **Data Structures** | Custom dynamic arrays & Hash tables |
| **Parsing Logic** | Canonical LR(1)

## Example

### Sample Input Code
```c
for (Init int i <- 0; i < max; i = i + 1) {
    if (registry.get_user(i).is_active =? true) {
        log_activity("User_" + i, true);
    }
}

```

### Program Tree Structure
The following represents the hierarchical structure of a standard `Program` node, showcasing a nested `for` loop and conditional logic.

```text
Program
└── CompStat
    └── UnitStat
        └── ControlStat
            └── LoopStat
                └── For
                    ├── Block
                    │   ├── }
                    │   ├── CompStat
                    │   │   └── UnitStat
                    │   │       └── ControlStat
                    │   │           └── CondStat
                    │   │               ├── Block
                    │   │               │   ├── }
                    │   │               │   ├── CompStat
                    │   │               │   │   └── UnitStat
                    │   │               │   │       ├── ;
                    │   │               │   │       └── Stat
                    │   │               │   │           └── Expr
                    │   │               │   │               └── Eval
                    │   │               │   │                   └── Term
                    │   │               │   │                       └── Factor
                    │   │               │   │                           └── Access
                    │   │               │   │                               ├── )
                    │   │               │   │                               ├── Args
                    │   │               │   │                               │   └── ArgList
                    │   │               │   │                               │       ├── Expr
                    │   │               │   │                               │       │   └── Eval
                    │   │               │   │                               │       │       └── Term
                    │   │               │   │                               │       │           └── Factor
                    │   │               │   │                               │       │               └── true
                    │   │               │   │                               │       ├── ,
                    │   │               │   │                               │       └── ArgList
                    │   │               │   │                               │           └── Expr
                    │   │               │   │                               │               └── Eval
                    │   │               │   │                               │                   ├── Term
                    │   │               │   │                               │                   │   └── Factor
                    │   │               │   │                               │                   │       └── Access
                    │   │               │   │                               │                   │           └── AccessBase
                    │   │               │   │                               │                   │               └── i
                    │   │               │   │                               │                   ├── +
                    │   │               │   │                               │                   └── Eval
                    │   │               │   │                               │                       └── Term
                    │   │               │   │                               │                           └── Factor
                    │   │               │   │                               │                               └── "User_"
                    │   │               │   │                               ├── (
                    │   │               │   │                               └── Access
                    │   │               │   │                                   └── AccessBase
                    │   │               │   │                                       └── log_activity
                    │   │               │   └── {
                    │   │               ├── )
                    │   │               ├── Expr
                    │   │               │   ├── Eval
                    │   │               │   │   └── Term
                    │   │               │   │       └── Factor
                    │   │               │   │           └── true
                    │   │               │   ├── LoP
                    │   │               │   │   └── =?
                    │   │               │   └── Expr
                    │   │               │       └── Eval
                    │   │               │           └── Term
                    │   │               │               └── Factor
                    │   │               │                   └── Access
                    │   │               │                       ├── is_active
                    │   │               │                       ├── .
                    │   │               │                       └── Access
                    │   │               │                           ├── )
                    │   │               │                           ├── Args
                    │   │               │                           │   └── ArgList
                    │   │               │                           │       └── Expr
                    │   │               │                           │           └── Eval
                    │   │               │                           │               └── Term
                    │   │               │                           │                   └── Factor
                    │   │               │                           │                       └── Access
                    │   │               │                           │                           └── AccessBase
                    │   │               │                           │                               └── i
                    │   │               │                           ├── (
                    │   │               │                           └── Access
                    │   │               │                               ├── get_user
                    │   │               │                               ├── .
                    │   │               │                               └── Access
                    │   │               │                                   └── AccessBase
                    │   │               │                                       └── registry
                    │   │               ├── (
                    │   │               └── if
                    │   └── {
                    ├── )
                    ├── Stat
                    │   └── Assignment
                    │       ├── Expr
                    │       │   └── Eval
                    │       │       ├── Term
                    │       │       │   └── Factor
                    │       │       │       └── 1
                    │       │       ├── +
                    │       │       └── Eval
                    │       │           └── Term
                    │       │               └── Factor
                    │       │                   └── Access
                    │       │                       └── AccessBase
                    │       │                           └── i
                    │       ├── =
                    │       └── Access
                    │           └── AccessBase
                    │               └── i
                    ├── ;
                    ├── Expr
                    │   ├── Eval
                    │   │   └── Term
                    │   │       └── Factor
                    │   │           └── Access
                    │   │               └── AccessBase
                    │   │                   └── max
                    │   ├── LoP
                    │   │   └── <
                    │   └── Expr
                    │       └── Eval
                    │           └── Term
                    │               └── Factor
                    │                   └── Access
                    │                       └── AccessBase
                    │                           └── i
                    ├── ;
                    ├── Stat
                    │   └── Declaration
                    │       ├── Expr
                    │       │   └── Eval
                    │       │       └── Term
                    │       │           └── Factor
                    │       │               └── 0
                    │       ├── <-
                    │       ├── i
                    │       ├── VarType
                    │       │   └── Primitive
                    │       │       └── int
                    │       └── Init
                    ├── (
                    └── for
```

## Lexical Specification and Grammar Rules

### Production Rules

| Non-Terminal | Production |
| :--- | :--- |
| **Goal** | `Program` |
| **Program** | `CompStat` |
| **Block** | `{ CompStat }` |
| **CompStat** | `UnitStat` \| `CompStat UnitStat` |
| **UnitStat** | `Stat ;` \| `ControlStat` |
| **CondStat** | `if ( Expr ) Block` \| `if ( Expr ) Block else Block` |
| **LoopStat** | `for ( Stat ; Expr ; Stat ) Block` |
| **Declaration** | `Init VarType name` \| `Init VarType name <- Expr` |
| **Assignment** | `Access = Expr` |
| **Expr** | `Expr LoP Eval` \| `Eval` |
| **Access** | `Access ( Args )` \| `Access . name` \| `AccessBase` |
| **LoP** | `=?` \| `<` \| `>` |


---

### Lexer String
```text
(=?)$19|(>=)$20|(<=)$21|(>)$22|(<)$23|+$07|-$08|/*$09|//$10|/($11|/)$12|/[$15|/]$16|.$17|,$18|(0|[1-9][0-9]*)$13|(\"([a-zA-Z0-9_][a-zA-Z0-9_]*)\")$24|(true)$25|(false)$26|(if)$32|(else)$33|(while)$34|(for)$35|(Init)$36|(Proc)$37|(return)$38|({)$39|(})$40|(;)$41|(<-)$42|(=)$43|(:)$44|(->)$45|(int)$46|(bool)$47|(float)$48|(break)$49|(continue)$50|(goto)$51|([a-zA-Z_][a-zA-Z0-9_]*)$14|(( |\n|\t|\r)( |\n|\t|\r)*)$01

```

| Pattern | ID | Description |
| :--- | :--- | :--- |
| `=?` | $19 | Equality Operator |
| `>=` | $20 | Greater Than or Equal |
| `<=` | $21 | Less Than or Equal |
| `>` | $22 | Greater Than |
| `<` | $23 | Less Than |
| `+` | $07 | Addition |
| `-` | $08 | Subtraction |
| `/*` | $09 | Block Comment Start |
| `//` | $10 | Line Comment |
| `(` | $11 | Left Parenthesis |
| `)` | $12 | Right Parenthesis |
| `[` | $15 | Left Bracket |
| `]` | $16 | Right Bracket |
| `.` | $17 | Dot / Accessor |
| `,` | $18 | Comma |
| `0 \| [1-9][0-9]*` | $13 | Integer Literal |
| `"([a-zA-Z0-9_]+)"` | $24 | String Literal |
| `true` | $25 | Boolean True |
| `false` | $26 | Boolean False |
| `if` | $32 | Keyword: If |
| `else` | $33 | Keyword: Else |
| `while` | $34 | Keyword: While |
| `for` | $35 | Keyword: For |
| `Init` | $36 | Keyword: Init |
| `Proc` | $37 | Keyword: Proc |
| `return` | $38 | Keyword: Return |
| `{` | $39 | Left Brace |
| `}` | $40 | Right Brace |
| `;` | $41 | Semicolon |
| `<-` | $42 | Left Arrow (Assignment/Init) |
| `=` | $43 | Assignment |
| `:` | $44 | Colon |
| `->` | $45 | Right Arrow |
| `int` | $46 | Type: Integer |
| `bool` | $47 | Type: Boolean |
| `float` | $48 | Type: Float |
| `break` | $49 | Keyword: Break |
| `continue` | $50 | Keyword: Continue |
| `goto` | $51 | Keyword: Goto |
| `[a-zA-Z_][a-zA-Z0-9_]*` | $14 | Identifier (Name) |
| `[ \n\t\r]+` | $01 | Whitespace (Ignored) |

### Grammar Lexer String
These rules are used specifically for defining the structural grammar rules.

```text
(([a-zA-Z/(/)/*///-/[/]+=?><.;{},:])([a-zA-Z/(/)/*///-/[/]+=?><.;{},:])*)$02|///|$03|(//->)$04|//;$05|(( |\n|\t|\r)( |\n|\t|\r)*)$01

```

| Pattern | Token ID | Description |
|:---|:---|:---|
| `(([a-zA-Z/()/ */ / /-/ /[]+=?><.;{},:])+)+` | `$02` | Grammar Symbol / Terminal |
| `\|` | `$03` | Grammar Choice (OR) |
| `->` | `$04` | Production Arrow |
| `;` | `$05` | Grammar Rule Terminator |
| `[ \n\t\r]+` | — | Whitespace (Ignored) |

---


