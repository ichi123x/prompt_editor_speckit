// TargetFolderState.h
// 対象フォルダから読み取った事実を表す構造。導入処理と診断の双方が参照する唯一の事実源。
// 設計根拠: data-model.md「1. TargetFolderState」

#pragma once

#include <afxstr.h>

// 002 FR-101: プロジェクト種別を表す列挙は廃止した。
// 種別の判定は本ツールの役割ではなく、/speckit-constitution が憲章を制定する
// 段階で行われる。マーカーは種別を区別せず1つの集合として扱う (002 FR-103)。

// 対象フォルダの Git の状態 (001 data-model.md「GitState」/ 001 research.md R-006)。
enum class GitState
{
	NotRepository,      // Gitリポジトリではない (git rev-parse が非0終了)
	RepositoryAtTarget, // 対象フォルダ自身がリポジトリのルート
	NestedUnderParent   // 対象フォルダが既存リポジトリのサブフォルダ (FR-016 の警告対象)
};

struct TargetFolderState
{
	CString path;                  // 対象フォルダの完全パス。未選択なら空
	// 認識可能なプロジェクトマーカーが1つ以上あるか (002 FR-103)。
	// 探すのは *.sln / *.vcxproj / *.csproj / package.json で、001 と同じ集合。
	// 種別ごとに分けて保持するのをやめ、1つのフラグへ統合した。
	bool hasProjectMarker = false;
	bool hasSpecifyDir = false;    // .specify が存在するか
	bool hasClaudeDir = false;     // .claude が存在するか
	bool hasGitignore = false;     // .gitignore が存在するか
	GitState gitState = GitState::NotRepository;
	CString gitRoot;               // リポジトリのルート。非リポジトリなら空
	int commitCount = 0;           // コミット件数。取得できなければ 0

	bool IsSelected() const { return !path.IsEmpty(); }

	// 足場が中途半端な状態か (.specify と .claude の一方だけが存在する)。
	// エッジケースとして警告の対象になる。
	bool HasPartialScaffold() const { return hasSpecifyDir != hasClaudeDir; }
};

namespace TargetFolder
{
	// 対象フォルダの状態を読み取る。ファイルの作成・変更・削除は一切行わないため、
	// 診断 (FR-020 の読み取り専用要件) からも安全に呼び出せる。
	TargetFolderState Read(const CString& folderPath);

	// 対象フォルダへの書き込み権限を確認する (001 FR-015)。
	// 一時ファイルの作成と削除を伴うため、診断からは呼び出してはならない (001 FR-020)。
	bool IsWritable(const CString& folderPath);
}
