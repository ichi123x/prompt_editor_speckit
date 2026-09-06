---

description: "機能実装用のタスクリストテンプレート"
---

# Tasks: [FEATURE NAME]

**Input**: `/specs/[###-feature-name]/` からの設計ドキュメント

**Prerequisites（前提条件）**: plan.md（必須）、spec.md（ユーザーストーリーに必須）、research.md、data-model.md、contracts/

**Tests（テスト）**: 以下の例にはテストタスクが含まれています。テストは任意であり、機能仕様で明示的に要求されている場合にのみ含めてください。

**Organization（構成）**: タスクはユーザーストーリーごとにグループ化され、各ストーリーの独立した実装とテストを可能にします。

## Format（形式）: `[ID] [P?] [Story] Description`

- **[P]**: 並行実行可能（異なるファイル、依存関係なし）
- **[Story]**: このタスクがどのユーザーストーリーに属するか（例: US1、US2、US3）
- 説明には正確なファイルパスを含めること

## Path Conventions（パスの慣例）

- **単一プロジェクト**: リポジトリルート直下の `src/`、`tests/`
- **Webアプリ**: `backend/src/`、`frontend/src/`
- **モバイル**: `api/src/`、`ios/src/` または `android/src/`
- 以下のパスは単一プロジェクトを前提としています - plan.md の構造に応じて調整してください

<!--
  ============================================================================
  重要: 以下のタスクは例示のためのサンプルタスクです。

  /speckit-tasks コマンドは、以下に基づいて実際のタスクに置き換えなければなりません:
  - spec.md のユーザーストーリー（優先度 P1、P2、P3...を含む）
  - plan.md の機能要件
  - data-model.md のエンティティ
  - contracts/ のエンドポイント

  タスクはユーザーストーリーごとに整理し、各ストーリーが以下を満たせるようにしなければなりません:
  - 独立して実装可能
  - 独立してテスト可能
  - MVPの増分として提供可能

  生成された tasks.md ファイルにこれらのサンプルタスクを残してはいけません。
  ============================================================================
-->

## Phase 1: Setup（共有インフラ）

**目的**: プロジェクトの初期化と基本構造

- [ ] T001 実装計画に沿ってプロジェクト構造を作成する
- [ ] T002 [language] プロジェクトを [framework] 依存関係で初期化する
- [ ] T003 [P] Lint とフォーマットツールを設定する

---

## Phase 2: Foundational（ブロッキングとなる前提条件）

**目的**: いずれのユーザーストーリーの実装前にも完了していなければならないコアインフラ

**⚠️ 重要**: このフェーズが完了するまで、どのユーザーストーリーの作業も開始できません

基盤タスクの例（プロジェクトに応じて調整してください）:

- [ ] T004 データベーススキーマとマイグレーションフレームワークをセットアップする
- [ ] T005 [P] 認証/認可フレームワークを実装する
- [ ] T006 [P] APIルーティングとミドルウェア構造をセットアップする
- [ ] T007 すべてのストーリーが依存する基本モデル/エンティティを作成する
- [ ] T008 エラー処理とロギングのインフラを設定する
- [ ] T009 環境設定の管理をセットアップする

**チェックポイント**: 基盤が完了 - これでユーザーストーリーの実装を並行して開始できます

---

## Phase 3: User Story 1 - [タイトル] (Priority: P1) 🎯 MVP

**目標**: [このストーリーが提供する内容の簡単な説明]

**独立したテスト**: [このストーリー単体で動作することをどう検証するか]

### User Story 1 のテスト（任意 - テストが要求された場合のみ） ⚠️

> **注記: これらのテストを先に書き、実装前に必ず失敗することを確認すること**

- [ ] T010 [P] [US1] tests/contract/test_[name].py に [endpoint] のコントラクトテストを書く
- [ ] T011 [P] [US1] tests/integration/test_[name].py に [user journey] の統合テストを書く

### User Story 1 の実装

- [ ] T012 [P] [US1] src/models/[entity1].py に [Entity1] モデルを作成する
- [ ] T013 [P] [US1] src/models/[entity2].py に [Entity2] モデルを作成する
- [ ] T014 [US1] src/services/[service].py に [Service] を実装する（T012、T013 に依存）
- [ ] T015 [US1] src/[location]/[file].py に [endpoint/feature] を実装する
- [ ] T016 [US1] バリデーションとエラー処理を追加する
- [ ] T017 [US1] ユーザーストーリー1の操作に対するログを追加する

**チェックポイント**: この時点で、User Story 1 は独立して完全に機能し、テスト可能であるはずです

---

## Phase 4: User Story 2 - [タイトル] (Priority: P2)

**目標**: [このストーリーが提供する内容の簡単な説明]

**独立したテスト**: [このストーリー単体で動作することをどう検証するか]

### User Story 2 のテスト（任意 - テストが要求された場合のみ） ⚠️

- [ ] T018 [P] [US2] tests/contract/test_[name].py に [endpoint] のコントラクトテストを書く
- [ ] T019 [P] [US2] tests/integration/test_[name].py に [user journey] の統合テストを書く

### User Story 2 の実装

- [ ] T020 [P] [US2] src/models/[entity].py に [Entity] モデルを作成する
- [ ] T021 [US2] src/services/[service].py に [Service] を実装する
- [ ] T022 [US2] src/[location]/[file].py に [endpoint/feature] を実装する
- [ ] T023 [US2]（必要であれば）User Story 1 のコンポーネントと統合する

**チェックポイント**: この時点で、User Story 1 と 2 の両方が独立して動作するはずです

---

## Phase 5: User Story 3 - [タイトル] (Priority: P3)

**目標**: [このストーリーが提供する内容の簡単な説明]

**独立したテスト**: [このストーリー単体で動作することをどう検証するか]

### User Story 3 のテスト（任意 - テストが要求された場合のみ） ⚠️

- [ ] T024 [P] [US3] tests/contract/test_[name].py に [endpoint] のコントラクトテストを書く
- [ ] T025 [P] [US3] tests/integration/test_[name].py に [user journey] の統合テストを書く

### User Story 3 の実装

- [ ] T026 [P] [US3] src/models/[entity].py に [Entity] モデルを作成する
- [ ] T027 [US3] src/services/[service].py に [Service] を実装する
- [ ] T028 [US3] src/[location]/[file].py に [endpoint/feature] を実装する

**チェックポイント**: これですべてのユーザーストーリーが独立して機能するはずです

---

[必要に応じて、同じパターンでユーザーストーリーのフェーズを追加してください]

---

## Phase N: Polish & Cross-Cutting Concerns（仕上げと横断的関心事）

**目的**: 複数のユーザーストーリーに影響する改善

- [ ] TXXX [P] docs/ のドキュメントを更新する
- [ ] TXXX コードのクリーンアップとリファクタリング
- [ ] TXXX すべてのストーリーにまたがるパフォーマンス最適化
- [ ] TXXX [P]（要求された場合）tests/unit/ に追加の単体テストを書く
- [ ] TXXX セキュリティの強化
- [ ] TXXX quickstart.md の検証を実行する

---

## Dependencies & Execution Order（依存関係と実行順序）

### Phase Dependencies（フェーズの依存関係）

- **Setup (Phase 1)**: 依存関係なし - すぐに開始可能
- **Foundational (Phase 2)**: Setup の完了に依存 - すべてのユーザーストーリーをブロックする
- **User Stories (Phase 3+)**: すべて Foundational フェーズの完了に依存する
  - （人員がいれば）その後ユーザーストーリーは並行して進められる
  - または優先度順（P1 → P2 → P3）に順次進められる
- **Polish (Final Phase)**: 対象とするすべてのユーザーストーリーの完了に依存する

### User Story Dependencies（ユーザーストーリーの依存関係）

- **User Story 1 (P1)**: Foundational (Phase 2) の後に開始可能 - 他のストーリーへの依存なし
- **User Story 2 (P2)**: Foundational (Phase 2) の後に開始可能 - US1 と統合する場合があるが、独立してテスト可能であるべき
- **User Story 3 (P3)**: Foundational (Phase 2) の後に開始可能 - US1/US2 と統合する場合があるが、独立してテスト可能であるべき

### Within Each User Story（各ユーザーストーリー内）

- テスト（含める場合）は実装前に書かれ、必ず失敗すること
- サービスの前にモデル
- エンドポイントの前にサービス
- 統合の前にコア実装
- 次の優先度に進む前にストーリーを完了させる

### Parallel Opportunities（並行実行の機会）

- [P] とマークされたすべての Setup タスクは並行実行可能
- [P] とマークされたすべての Foundational タスクは（Phase 2 内で）並行実行可能
- Foundational フェーズが完了すれば、（チームの人数が許せば）すべてのユーザーストーリーを並行して開始できる
- [P] とマークされた1つのユーザーストーリー内のすべてのテストは並行実行可能
- 1つのストーリー内で [P] とマークされたモデルは並行実行可能
- 異なるユーザーストーリーは異なるチームメンバーが並行して作業できる

---

## Parallel Example: User Story 1（並行実行の例）

```bash
# User Story 1 のすべてのテストを一緒に実行する（テストが要求された場合）:
Task: "tests/contract/test_[name].py に [endpoint] のコントラクトテストを書く"
Task: "tests/integration/test_[name].py に [user journey] の統合テストを書く"

# User Story 1 のすべてのモデルを一緒に実行する:
Task: "src/models/[entity1].py に [Entity1] モデルを作成する"
Task: "src/models/[entity2].py に [Entity2] モデルを作成する"
```

---

## Implementation Strategy（実装戦略）

### MVP First（User Story 1 のみ）

1. Phase 1: Setup を完了する
2. Phase 2: Foundational を完了する（重要 - すべてのストーリーをブロックする）
3. Phase 3: User Story 1 を完了する
4. **停止して検証**: User Story 1 を単独でテストする
5. 準備ができればデプロイ/デモする

### Incremental Delivery（段階的な提供）

1. Setup + Foundational を完了する → 基盤が準備完了
2. User Story 1 を追加する → 単独でテストする → デプロイ/デモ（MVP!）
3. User Story 2 を追加する → 単独でテストする → デプロイ/デモ
4. User Story 3 を追加する → 単独でテストする → デプロイ/デモ
5. 各ストーリーは、これまでのストーリーを壊さずに価値を追加する

### Parallel Team Strategy（並行チーム戦略）

複数の開発者がいる場合:

1. チーム全体で Setup + Foundational を完了する
2. Foundational が完了したら:
   - 開発者A: User Story 1
   - 開発者B: User Story 2
   - 開発者C: User Story 3
3. ストーリーは独立して完了・統合される

---

## Notes（注記）

- [P] タスク = 異なるファイル、依存関係なし
- [Story] ラベルは、トレーサビリティのためにタスクを特定のユーザーストーリーに対応付ける
- 各ユーザーストーリーは独立して完了・テスト可能であるべき
- 実装前にテストが失敗することを確認する
- 各タスクまたは論理的なまとまりごとにコミットする
- 各チェックポイントで一旦止まり、ストーリーが独立していることを検証する
- 避けるべきこと: 曖昧なタスク、同一ファイルへの競合、独立性を損なうストーリー間の依存関係
