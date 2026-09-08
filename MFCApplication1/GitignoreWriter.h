// GitignoreWriter.h
// 埋め込みリソースから .gitignore を生成し、追記ブロックをべき等に適用する。
// 設計根拠: research.md R-007 / contracts/ui-contract.md「6. 生成されるファイルのコントラクト」

#pragma once

#include <afxstr.h>
#include "TargetFolderState.h"

// 追記ブロックの目印行。この行の有無で追記済みかを判定するため、
// 文字列を変更すると既存の .gitignore が未追記とみなされ、ブロックが重複する。
#define GITIGNORE_MARK_SPECKIT   _T("# ---- Spec Kit ----")
#define GITIGNORE_MARK_PROJECT   _T("# ---- このプロジェクト固有 ----")

namespace GitignoreWriter
{
	// 対象フォルダに .gitignore を用意する (001 FR-010 / 002 FR-106, FR-106a, FR-106b)。
	//
	//  - .gitignore が無ければ、統合された埋め込みリソース (IDR_GITIGNORE) の本体を
	//    書き出し、続けて2つの追記ブロックを付与する。書き出す内容は対象フォルダの
	//    内容に依存せず常に同一である (002 FR-106)。
	//  - 既に存在する場合は本体を書き換えず、目印行が無いブロックのみを末尾へ追記する。
	//    既存の本体が統合版と異なっていても、統合版で置き換えてはならない (002 FR-106a)。
	//  - 何度実行してもブロックが重複しない (002 FR-106b)。
	//
	// changed には、ファイルを作成または追記した場合に true が入る。
	// 既に完全な状態で何もしなかった場合は false。
	bool Ensure(const CString& folderPath, bool& changed, CString& errorMessage);
}
