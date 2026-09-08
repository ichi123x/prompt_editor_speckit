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
	// 対象フォルダに .gitignore を用意する (FR-010)。
	//
	//  - .gitignore が無ければ、種別に応じた埋め込みリソースの本体を書き出し、
	//    続けて2つの追記ブロックを付与する。
	//  - 既に存在する場合は本体を書き換えず、目印行が無いブロックのみを末尾へ追記する。
	//  - 何度実行してもブロックが重複しない (FR-013)。
	//
	// changed には、ファイルを作成または追記した場合に true が入る。
	// 既に完全な状態で何もしなかった場合は false。
	bool Ensure(const CString& folderPath, ProjectKind kind, bool& changed, CString& errorMessage);
}
