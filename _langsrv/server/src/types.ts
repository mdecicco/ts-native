import { Diagnostic } from 'vscode-languageserver';
import { TextDocument } from 'vscode-languageserver-textdocument';

export namespace TSN {
	export type NodeType =
	'empty' | 'root' | 'eos' | 'break' | 'cast' | 'catch' | 'class' | 'continue' | 'export' |
	'expression' | 'function_type' | 'function' | 'identifier' | 'if' | 'import_module' | 'import_symbol' |
	'import' | 'literal' | 'loop' | 'object_decompositor' | 'object_literal_property' | 'parameter' |
	'property' | 'return' | 'scoped_block' | 'sizeof' | 'switch_case' | 'switch' | 'this' | 'throw' | 'try' |
	'type_modifier' | 'type_property' | 'type_specifier' | 'type' | 'variable';

	export type Operation =
	'undefined' | 'add' | 'addEq' | 'sub' | 'subEq' | 'mul' | 'mulEq' | 'div' | 'divEq' | 'mod' | 'modEq' |
	'xor' | 'xorEq' | 'bitAnd' | 'bitAndEq' | 'bitOr' | 'bitOrEq' | 'bitInv' | 'shLeft' | 'shLeftEq' | 'shRight'
	| 'shRightEq' | 'not' | 'notEq' | 'logAnd' | 'logAndEq' | 'logOr' | 'logOrEq' | 'assign' | 'compare' |
	'lessThan' | 'lessThanEq' | 'greaterThan' | 'greaterThanEq' | 'preInc' | 'postInc' | 'preDec' | 'postDec' |
	'negate' | 'index' | 'conditional' | 'member' | 'new' | 'placementNew' | 'call';

	export type LiteralType =
	'u8' | 'u16' | 'u32' | 'u64' | 'i8' | 'i16' | 'i32' | 'i64' | 'f32' | 'f64' | 'string' | 'template_string' |
	'object' | 'array' | 'null' | 'true' | 'false';

	export type NodeFlags = 'is_const' | 'is_static' | 'is_private' | 'is_array' | 'is_pointer' | 'defer_cond';

	export type CodeRange = {
		start: {
			line: number;
			col: number;
			offset: number;
		};
		end: {
			line: number;
			col: number;
			offset: number;
		};
	};

	export type ASTNode = {
		type: NodeType;
		source: {
			range: CodeRange;
			line_text: string;
			indx_text: string;
		} | null;
		value_type?: LiteralType;
		value?: any;
		text?: string;
		operation?: Operation;
		flags: NodeFlags[];
		data_type: ASTNode | ASTNode[] | null;
		lvalue: ASTNode | ASTNode[] | null;
		rvalue: ASTNode | ASTNode[] | null;
		cond: ASTNode | ASTNode[] | null;
		body: ASTNode | ASTNode[] | null;
		else_body: ASTNode | ASTNode[] | null;
		initializer: ASTNode | ASTNode[] | null;
		parameters: ASTNode | ASTNode[] | null;
		template_parameters: ASTNode | ASTNode[] | null;
		modifier: ASTNode | ASTNode[] | null;
		alias: ASTNode | ASTNode[] | null;
		extends: ASTNode | ASTNode[] | null;
	};

	export type CompilerLog = {
		code: string | null;
		type: 'info' | 'warning' | 'error';
		range: CodeRange | null;
		line_txt: string | null;
		line_idx: string | null;
		message: string;
		ast: ASTNode | ASTNode[] | null;
	};

	export type AccessModifier = 'public' | 'private';

	export type BriefFunctionInfo = {
		name: string;
		fully_qualified_name: string;
		access: AccessModifier;
	};

	export type TypeFlags = 
	'is_pod' | 'is_trivially_constructible' | 'is_trivially_copyable' | 'is_trivially_destructible' |
	'is_primitive' | 'is_floating_point' | 'is_integral' | 'is_unsigned' | 'is_function' | 'is_template' |
	'is_alias' | 'is_host' | 'is_anonymous';

	export type TypeBase = {
		type: string;
		access: AccessModifier;
		data_offset: number;
	};

	export type TypePropertyFlags = 'is_static' | 'is_pointer' | 'can_read' | 'can_write';

	export type TypeProperty = {
		name: string;
		type: string;
		offset: number;
		access: AccessModifier;
		getter: string | null;
		setter: string | null;
		flags: TypePropertyFlags[];
	};

	export type TypeInfo = {
		id: number;
		size: number;
		host_hash: number;
		name: string;
		fully_qualified_name: string;
		access: AccessModifier;
		flags: TypeFlags[];
		destructor: BriefFunctionInfo | null;
		alias_of: string | null;
		inherits: TypeBase[];
		properties: TypeProperty[];
		methods: BriefFunctionInfo[];
	};

	export type FunctionArgType = 'func_ptr' | 'ret_ptr' | 'ectx_ptr' | 'this_ptr' | 'value' | 'pointer';

	export type FunctionArg = {
		arg_type: FunctionArgType;
		is_implicit: boolean;
		data_type: string;
		location: string;
	};

	export type FunctionInfo = {
		id: number;
		name: string;
		fully_qualified_name: string;
		signature: string;
		access: AccessModifier;
		is_method: boolean;
		is_thiscall: boolean;
		args: FunctionArg[];
		code: string[];
	};

	export type GlobalInfo = {
		name: string;
		access: AccessModifier;
		type: string;
	};

	export type SymbolType = 'value' | 'function' | 'module' | 'type';

	export type SymbolRange = {
		name: string;
		type: SymbolType;
		detail: string;
		range: CodeRange;
	};

	export type CompilerResult = {
		logs: CompilerLog[];
		symbols: SymbolRange[];
		types: TypeInfo[];
		functions: FunctionInfo[];
		globals: GlobalInfo[];
		ast: ASTNode | null;
	};

	export type ValidationResult = {
		document: TextDocument;
		diagnostics: Diagnostic[];
	};

	export interface Settings {
		maxNumberOfProblems: number;
	};
};