/* --------------------------------------------------------------------------------------------
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See License.txt in the project root for license information.
 * ------------------------------------------------------------------------------------------ */
import {
	createConnection,
	TextDocuments,
	ProposedFeatures,
	InitializeParams,
	DidChangeConfigurationNotification,
	CompletionItem,
	CompletionItemKind,
	TextDocumentPositionParams,
	TextDocumentSyncKind,
	InitializeResult,
	DocumentLink
} from 'vscode-languageserver/node';

import { TextDocument } from 'vscode-languageserver-textdocument';

import { TSN } from './types';
import { ScriptManager } from './language_processor';
import { BuiltinTypes, Keywords } from './language';

const connection = createConnection(ProposedFeatures.all);
const documents: TextDocuments<TextDocument> = new TextDocuments(TextDocument);

const Messager = {
	info: (msg: string) : void => {
		connection.window.showInformationMessage(msg);
	},
	warn: (msg: string) : void => {
		connection.window.showWarningMessage(msg);
	},
	error: (msg: string) : void => {
		connection.window.showErrorMessage(msg);
	},
	validation: (result: TSN.ValidationResult) : void => {
		connection.sendDiagnostics({
			uri: result.document.uri,
			diagnostics: result.diagnostics
		});
	}
};
let scriptMgr : ScriptManager | null = null;

connection.onInitialize((params: InitializeParams) => {
	const result: InitializeResult = {
		capabilities: {
			textDocumentSync: TextDocumentSyncKind.Full,
			// Tell the client that this server supports code completion.
			completionProvider: {
				resolveProvider: true
			},
			documentLinkProvider: {
				resolveProvider: true
			}
		}
	};
	return result;
});

connection.onInitialized(() => {
	scriptMgr = new ScriptManager(documents, Messager);
	connection.client.register(DidChangeConfigurationNotification.type, undefined);
});

connection.onDidChangeConfiguration(change => {
	if (scriptMgr) scriptMgr.onSettingsChanged(change.settings.tsppServer as TSN.Settings);
});

// This handler provides the initial list of the completion items.
connection.onCompletion(
	(pos: TextDocumentPositionParams): CompletionItem[] => {
		if (!scriptMgr) return [];
		// The pass parameter contains the position of the text document in
		// which code complete got requested. For the example we ignore this
		// info and always provide the same completion items.

		try {
			const completions = scriptMgr.getCompletions(pos.textDocument.uri, pos.position);

			return completions.concat(Keywords.concat(BuiltinTypes))
		} catch (e) {
			Messager.error((e as Error).toString());
			Messager.info((e as Error).stack || '');
		}

		return [];
	}
);

connection.onCompletionResolve((item: CompletionItem): CompletionItem => {
	return item;
});

connection.onDocumentLinks((params, token, workdoneprogress, resultprogress) : DocumentLink[] => {
	
	return [];
});

documents.listen(connection);
connection.listen();