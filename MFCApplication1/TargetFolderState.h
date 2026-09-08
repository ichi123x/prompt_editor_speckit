// TargetFolderState.h
// 対象フォルダから読み取った事実を表す構造。導入処理と診断の双方が参照する唯一の事実源。
// 設計根拠: data-model.md「1. TargetFolderState」

#pragma once

#include <afxstr.h>

// ユーザーが選択する対象の種別 (data-model.md「2. ProjectKind」)。
enum class ProjectKind
{
	None,     // 未選択。この状態で「生成」を押した場合は中止する (FR-005)
	MfcApp,   // MFCアプリ。マーカーは *.sln / *.vcxproj
	WebApp    // Webアプリ。マーカーは package.json / *.csproj
};

// 対象フォルダの Git の状態 (data-model.md「GitState」/ research.md R-006)。
enum class GitState
{
	NotRepository,      // Gitリポジトリではない (git rev-parse が非0終了)
	RepositoryAtTarget, // 対象フォルダ自身がリポジトリのルート
	NestedUnderParent   // 対象フォルダが既存リポジトリのサブフォルダ (FR-016 の警告対象)
};

struct TargetFolderState
{
	CString path;                  // 対象フォルダの完全パス。未選択なら空
	bool hasSlnMarker = false;     // *.sln / *.vcxproj が存在するか
	bool hasWebMarker = false;     // package.json / *.csproj が存在するか
	bool hasSpecifyDir = false;    // .specify が存在するか
	bool hasClaudeDir = false;     // .claude が存在するか
	bool hasGitignore = false;     // .gitignore が存在するか
	GitState gitState = GitState::NotRepository;
	CString gitRoot;               // リポジトリのルート。非リポジトリなら空
	int commitCount = 0;           // コミット件数。取得できなければ 0

	bool IsSelected() const { return !path.IsEmpty(); }

	// 指定された種別に対応するマーカーがあるか (FR-006)。
	bool HasMarkerFor(ProjectKind kind) const;

	// 足場が中途半端な状態か (.specify と .claude の一方だけが存在する)。
	// エッジケースとして警告の対象になる。
	bool HasPartialScaffold() const { return hasSpecifyDir != hasClaudeDir; }
};

namespace TargetFolder
{
	// 対象フォルダの状態を読み取る。ファイルの作成・変更・削除は一切行わないため、
	// 診断 (FR-020 の読み取り専用要件) からも安全に呼び出せる。
	TargetFolderState Read(const CString& folderPath);

	// 対象フォルダへの書き込み権限を確認する (FR-015)。
	// 一時ファイルの作成と削除を伴うため、診断からは呼び出してはならない (FR-020)。
	bool IsWritable(const CString& folderPath);

	// 種別の表示名を返す。
	CString KindDisplayName(ProjectKind kind);
}
