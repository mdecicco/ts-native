import { CompletionItem, CompletionItemKind } from 'vscode-languageserver';

export const Keywords : CompletionItem[] = [
	{ label: 'if'      , kind: CompletionItemKind.Keyword },
	{ label: 'else'    , kind: CompletionItemKind.Keyword },
	{ label: 'do'      , kind: CompletionItemKind.Keyword },
	{ label: 'while'   , kind: CompletionItemKind.Keyword },
	{ label: 'for'     , kind: CompletionItemKind.Keyword },
	{ label: 'break'   , kind: CompletionItemKind.Keyword },
	{ label: 'continue', kind: CompletionItemKind.Keyword },
	{ label: 'type'    , kind: CompletionItemKind.Keyword },
	{ label: 'enum'    , kind: CompletionItemKind.Keyword },
	{ label: 'class'   , kind: CompletionItemKind.Keyword },
	{ label: 'extends' , kind: CompletionItemKind.Keyword },
	{ label: 'public'  , kind: CompletionItemKind.Keyword },
	{ label: 'private' , kind: CompletionItemKind.Keyword },
	{ label: 'import'  , kind: CompletionItemKind.Keyword },
	{ label: 'export'  , kind: CompletionItemKind.Keyword },
	{ label: 'from'    , kind: CompletionItemKind.Keyword },
	{ label: 'as'      , kind: CompletionItemKind.Keyword },
	{ label: 'operator', kind: CompletionItemKind.Keyword },
	{ label: 'static'  , kind: CompletionItemKind.Keyword },
	{ label: 'const'   , kind: CompletionItemKind.Keyword },
	{ label: 'get'     , kind: CompletionItemKind.Keyword },
	{ label: 'set'     , kind: CompletionItemKind.Keyword },
	{ label: 'null'    , kind: CompletionItemKind.Constant },
	{ label: 'return'  , kind: CompletionItemKind.Keyword },
	{ label: 'switch'  , kind: CompletionItemKind.Keyword },
	{ label: 'case'    , kind: CompletionItemKind.Keyword },
	{ label: 'default' , kind: CompletionItemKind.Keyword },
	{ label: 'true'    , kind: CompletionItemKind.Constant },
	{ label: 'false'   , kind: CompletionItemKind.Constant },
	{ label: 'this'    , kind: CompletionItemKind.Keyword },
	{ label: 'function', kind: CompletionItemKind.Keyword },
	{ label: 'let'     , kind: CompletionItemKind.Keyword },
	{ label: 'new'     , kind: CompletionItemKind.Keyword },
	{ label: 'sizeof'  , kind: CompletionItemKind.Keyword, detail: 'sizeof<T>' }
];

export const BuiltinTypes : CompletionItem[] = [
	{
		label: 'string',
		kind: CompletionItemKind.Class,
		detail: 'A string of text'
	}, {
		label: 'i8',
		kind: CompletionItemKind.Class,
		detail: '8 bit signed integer'
	}, {
		label: 'i16',
		kind: CompletionItemKind.Class,
		detail: '16 bit signed integer'
	}, {
		label: 'i32',
		kind: CompletionItemKind.Class,
		detail: '32 bit signed integer'
	}, {
		label: 'i64',
		kind: CompletionItemKind.Class,
		detail: '64 bit signed integer'
	}, {
		label: 'u8',
		kind: CompletionItemKind.Class,
		detail: '8 bit unsigned integer'
	}, {
		label: 'u16',
		kind: CompletionItemKind.Class,
		detail: '16 bit unsigned integer'
	}, {
		label: 'u32',
		kind: CompletionItemKind.Class,
		detail: '32 bit unsigned integer'
	}, {
		label: 'u64',
		kind: CompletionItemKind.Class,
		detail: '64 bit unsigned integer'
	}, {
		label: 'f32',
		kind: CompletionItemKind.Class,
		detail: '32 bit floating point number'
	}, {
		label: 'f64',
		kind: CompletionItemKind.Class,
		detail: '64 bit floating point number'
	}, {
		label: 'void',
		kind: CompletionItemKind.Class,
		detail: 'Represents nothing'
	}, {
		label: 'data',
		kind: CompletionItemKind.Class,
		detail: 'Pointer to memory address'
	}, {
		label: 'boolean',
		kind: CompletionItemKind.Class,
		detail: 'True or false'
	},
];