import { CompletionItem, CompletionItemKind, DiagnosticRelatedInformation, DiagnosticSeverity, TextDocumentChangeEvent, TextDocuments } from 'vscode-languageserver';
import {
	Position,
	Range,
	TextDocument
} from 'vscode-languageserver-textdocument';

import { exec } from 'child_process';
import { TSN } from './types';

export class LanguageProcessor {
	private file : TextDocument;
	private mgr : ScriptManager;
	private last : TSN.CompilerResult | null;
	private tempFilePath : string;

	constructor (file: TextDocument, mgr: ScriptManager) {
		this.file = file;
		this.mgr = mgr;
		this.last = null;
		this.tempFilePath = '';
	}

	getUri () {
		return this.file.uri;
	}

	getFullRange () : Range {
		return {
			start: this.file.positionAt(0),
			end: this.file.positionAt(this.file.getText().length)
		};
	}

	getLogRange (log: TSN.CompilerLog) : Range {
		if (log.range) {
			return {
				start: { line: log.range.start.line, character: log.range.start.col },
				end: { line: log.range.end.line, character: log.range.end.col },
			};
		}

		return this.getFullRange();
	}

	getLastResult () : TSN.CompilerResult | null {
		return this.last;
	}

	validate () : Promise<TSN.ValidationResult> {
		return new Promise<TSN.ValidationResult>((resolve) => {
			const output : TSN.ValidationResult = {
				document: this.file,
				diagnostics: []
			};
	
			let docPath = this.file.uri.substring(8).replace('%3A', ':');
	
			try {
				exec(`C:/Users/miguel/programming/gjs/_bin/Debug/gs2json.exe "${docPath}"`, (err, stdout, stderr) => {
					if (err) {
						this.mgr.error(err.message);
						resolve(output);
					} else {
						try {
							const result = JSON.parse(stdout) as TSN.CompilerResult;
							const settings = this.mgr.getSettings();

							for (let i = 0;i < result.logs.length && i < settings.maxNumberOfProblems;i++) {
								const l = result.logs[i];

								let severity : DiagnosticSeverity = DiagnosticSeverity.Information;
								if (l.type === 'warning') severity = DiagnosticSeverity.Warning;
								else if (l.type === 'error') severity = DiagnosticSeverity.Error;

								let info : DiagnosticRelatedInformation[] = [];
								if (i < result.logs.length - 1) {
									for (let n = i + 1;n < result.logs.length;n++) {
										const ln = result.logs[n];
										if (ln.type === 'info' && ln.message.length > 0 && ln.message[0] === '^') {
											info.push({
												location: {
													range: this.getLogRange(ln),
													uri: this.file.uri
												},
												message: ln.message.substring(2)
											});
											i++;
											continue;
										}

										break;
									}
								}

								output.diagnostics.push({
									code: l.code || '',
									relatedInformation: info,
									severity,
									range: this.getLogRange(l),
									message: l.message,
									source: 'tsn-compiler'
								});
							}

							this.last = result;
						} catch (e) {
							this.mgr.error((e as Error).toString());
							this.mgr.log((e as Error).stack || '');
							this.mgr.log(stdout);
							this.mgr.log(stderr);
							resolve(output);
						}
					}
					
					resolve(output);
				});
			} catch (e) {
				this.mgr.error((e as Error).toString());
				this.mgr.log((e as Error).stack || '');
				resolve(output);
			}
		});
	}
};

interface Messager {
	info(msg: string) : void;
	warn(msg: string) : void;
	error(msg: string) : void;
	validation(results: TSN.ValidationResult) : void;
};

export class ScriptManager {
	private files : TextDocuments<TextDocument>;
	private processors : Map<string, LanguageProcessor>;
	private settings : TSN.Settings;
	private messager : Messager;

	constructor (files: TextDocuments<TextDocument>, messager: Messager) {
		this.files = files;
		this.processors = new Map<string, LanguageProcessor>();
		this.settings = {
			maxNumberOfProblems: 100
		};
		this.messager = messager;

		this.files.onDidClose(this.onFileClose.bind(this));
		this.files.onDidChangeContent(this.onFileChange.bind(this));
		this.files.onDidSave(this.onFileSave.bind(this));
		this.files.onDidOpen(this.onFileOpen.bind(this));
	}

	getSettings () {
		return this.settings;
	}

	log (msg: string) {
		this.messager.info(msg);
	}

	warn (msg: string) {
		this.messager.warn(msg);
	}

	error (msg: string) {
		this.messager.error(msg);
	}

	async onFileOpen (e: TextDocumentChangeEvent<TextDocument>) {
		const processor = new LanguageProcessor(e.document, this);
		this.processors.set(e.document.uri, processor);

		const result = await processor.validate();
		this.messager.validation(result);
	}

	onFileClose (e: TextDocumentChangeEvent<TextDocument>) {
		this.processors.delete(e.document.uri);
	}

	async onFileChange (e: TextDocumentChangeEvent<TextDocument>) {
		
	}

	async onFileSave (e: TextDocumentChangeEvent<TextDocument>) {
		const processor = this.processors.get(e.document.uri);
		if (processor) {
			const result = await processor.validate();
			this.messager.validation(result);
		}
	}
	
	onSettingsChanged (s: TSN.Settings) {
		this.settings = s;
		this.validateAll();
	}

	validateAll () {
		this.processors.forEach(p => {
			p.validate().then(result => {
				this.messager.validation(result);
			});
		});
	}

	getCompletions (uri: string, pos: Position) : CompletionItem[] {
		const processor = this.processors.get(uri);
		if (!processor) return [];

		const data = processor.getLastResult();
		if (!data) return [];

		const out: CompletionItem[] = [];

		// this.error(`pos: ${pos.line}:${pos.character}`);

		data.symbols.forEach(s => {
			const info = `line: ${s.range.start.line + 1}, col: ${s.range.start.col + 1} -> ${s.range.end.line + 1}, col: ${s.range.end.col + 1}`;

			if (s.name[0] === '@' || s.name[0] === '$') return;
			if (s.range.start.line >= pos.line || (s.range.start.line == pos.line && s.range.start.col >= pos.character)) return;
			if (s.range.end.line <= pos.line || (s.range.end.line == pos.line && s.range.end.col <= pos.character)) return;
			if (s.name.includes('operator')) return;
			if (s.name.includes('constructor')) return;
			if (s.name.includes('destructor')) return;

			let kind : CompletionItemKind = CompletionItemKind.Variable;
			if (s.type === 'function') kind = CompletionItemKind.Function;
			else if (s.type === 'module') kind = CompletionItemKind.Module;
			else if (s.type === 'type') kind = CompletionItemKind.Class;

			let detail = s.detail;
			// detail = info;

			out.push({
				label: s.name,
				detail,
				kind
			});
		});

		data.types.forEach(t => {
			if (t.flags.includes('is_anonymous') || t.flags.includes('is_function')) return;
			out.push({
				label: t.name,
				kind: CompletionItemKind.Class,
				detail: t.fully_qualified_name
			});
		});
		
		data.globals.forEach(g => {
			out.push({
				label: g.name,
				kind: CompletionItemKind.Value,
				detail: g.type
			});
		});

		return out;
	}
};