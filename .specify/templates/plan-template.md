# Implementation Plan: [FEATURE]

**Branch**: `[###-feature-name]` | **Date**: [DATE] | **Spec**: [link]

**Input**: `/specs/[###-feature-name]/spec.md` からの機能仕様

**注記**: このテンプレートは `/speckit-plan` コマンドによって記入されます。その定義には実行ワークフローが記載されています。

## Summary（概要）

[機能仕様から抽出: 主要な要件 + リサーチに基づく技術的アプローチ]

## Technical Context（技術コンテキスト）

<!--
  対応が必要: このセクションの内容を、このプロジェクトの技術詳細に置き換えてください。
  ここに示す構造は、反復作業を導くための参考として提示されています。
-->

**Language/Version**: [例: Python 3.11、Swift 5.9、Rust 1.75 または NEEDS CLARIFICATION]

**Primary Dependencies**: [例: FastAPI、UIKit、LLVM または NEEDS CLARIFICATION]

**Storage**: [該当する場合、例: PostgreSQL、CoreData、ファイル または N/A]

**Testing**: [例: pytest、XCTest、cargo test または NEEDS CLARIFICATION]

**Target Platform**: [例: Linuxサーバー、iOS 15+、WASM または NEEDS CLARIFICATION]

**Project Type**: [例: library/cli/web-service/mobile-app/compiler/desktop-app または NEEDS CLARIFICATION]

**Performance Goals**: [ドメイン固有、例: 1000 req/s、10k lines/sec、60 fps または NEEDS CLARIFICATION]

**Constraints**: [ドメイン固有、例: <200ms p95、<100MBメモリ、オフライン対応 または NEEDS CLARIFICATION]

**Scale/Scope**: [ドメイン固有、例: 1万ユーザー、100万行、50画面 または NEEDS CLARIFICATION]

## Constitution Check（憲法チェック）

*ゲート: Phase 0 のリサーチ前に通過する必要がある。Phase 1 の設計後に再チェックすること。*

[憲法ファイルに基づいて決定されるゲート]

## Project Structure（プロジェクト構造）

### Documentation（このフィーチャー）

```text
specs/[###-feature]/
├── plan.md              # このファイル（/speckit-plan コマンドの出力）
├── research.md          # Phase 0 の出力（/speckit-plan コマンド）
├── data-model.md        # Phase 1 の出力（/speckit-plan コマンド）
├── quickstart.md        # Phase 1 の出力（/speckit-plan コマンド）
├── contracts/           # Phase 1 の出力（/speckit-plan コマンド）
└── tasks.md             # Phase 2 の出力（/speckit-tasks コマンド - /speckit-plan では作成されない）
```

### Source Code（リポジトリのルート）
<!--
  対応が必要: 以下のプレースホルダーのツリーを、この機能の実際のレイアウトに置き換えてください。
  使わないオプションは削除し、選択した構造を実際のパス（例: apps/admin、packages/something）で展開してください。
  最終的なプランに Option ラベルを含めてはいけません。
-->

```text
# [使わない場合は削除] Option 1: 単一プロジェクト（デフォルト）
src/
├── models/
├── services/
├── cli/
└── lib/

tests/
├── contract/
├── integration/
└── unit/

# [使わない場合は削除] Option 2: Web アプリケーション（"frontend" + "backend" が検出された場合）
backend/
├── src/
│   ├── models/
│   ├── services/
│   └── api/
└── tests/

frontend/
├── src/
│   ├── components/
│   ├── pages/
│   └── services/
└── tests/

# [使わない場合は削除] Option 3: モバイル + API（"iOS/Android" が検出された場合）
api/
└── [上記の backend と同様]

ios/ または android/
└── [プラットフォーム固有の構造: フィーチャーモジュール、UIフロー、プラットフォームテスト]
```

**Structure Decision**: [選択した構造を文書化し、上記で記録した実際のディレクトリを参照すること]

## Complexity Tracking（複雑さの追跡）

> **Constitution Check に違反があり、正当化が必要な場合にのみ記入すること**

| Violation（違反） | Why Needed（必要な理由） | Simpler Alternative Rejected Because（よりシンプルな代替案を却下した理由） |
|-----------|------------|-------------------------------------|
| [例: 4つ目のプロジェクト] | [現在必要な理由] | [3プロジェクトでは不十分な理由] |
| [例: Repository パターン] | [具体的な問題] | [直接のDBアクセスでは不十分な理由] |
