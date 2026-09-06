# [PROJECT_NAME] Constitution
<!-- 例: Spec Constitution, TaskFlow Constitution など -->

## Core Principles（基本原則）

### [PRINCIPLE_1_NAME]
<!-- 例: I. Library-First -->
[PRINCIPLE_1_DESCRIPTION]
<!-- 例: すべての機能はスタンドアロンのライブラリとして開始する。ライブラリは自己完結的で、独立してテスト可能で、文書化されていなければならない。明確な目的が必要 - 組織上の理由だけのライブラリは不可 -->

### [PRINCIPLE_2_NAME]
<!-- 例: II. CLI Interface -->
[PRINCIPLE_2_DESCRIPTION]
<!-- 例: すべてのライブラリは CLI 経由で機能を公開する。テキストの入出力プロトコル: stdin/args → stdout、エラー → stderr。JSON と人間可読フォーマットの両方をサポート -->

### [PRINCIPLE_3_NAME]
<!-- 例: III. Test-First (NON-NEGOTIABLE) -->
[PRINCIPLE_3_DESCRIPTION]
<!-- 例: TDD必須: テスト作成 → ユーザー承認 → テスト失敗 → その後実装。Red-Green-Refactor サイクルを厳格に適用 -->

### [PRINCIPLE_4_NAME]
<!-- 例: IV. Integration Testing -->
[PRINCIPLE_4_DESCRIPTION]
<!-- 例: 統合テストが必要な重点領域: 新しいライブラリのコントラクトテスト、コントラクトの変更、サービス間通信、共有スキーマ -->

### [PRINCIPLE_5_NAME]
<!-- 例: V. Observability、VI. Versioning & Breaking Changes、VII. Simplicity -->
[PRINCIPLE_5_DESCRIPTION]
<!-- 例: テキストI/Oによりデバッグ可能性を確保。構造化ログが必須。または: MAJOR.MINOR.BUILD 形式。または: シンプルから始める、YAGNI原則 -->

## [SECTION_2_NAME]
<!-- 例: 追加制約、セキュリティ要件、パフォーマンス基準など -->

[SECTION_2_CONTENT]
<!-- 例: 技術スタックの要件、コンプライアンス基準、デプロイポリシーなど -->

## [SECTION_3_NAME]
<!-- 例: 開発ワークフロー、レビュープロセス、品質ゲートなど -->

[SECTION_3_CONTENT]
<!-- 例: コードレビュー要件、テストゲート、デプロイ承認プロセスなど -->

## Governance（ガバナンス）
<!-- 例: 憲法は他のすべての慣行に優先する。改訂には文書化、承認、移行計画が必要 -->

[GOVERNANCE_RULES]
<!-- 例: すべてのPR/レビューは準拠を確認しなければならない。複雑さは正当化されなければならない。実行時の開発ガイダンスには [GUIDANCE_FILE] を使用 -->

**Version**: [CONSTITUTION_VERSION] | **Ratified**: [RATIFICATION_DATE] | **Last Amended**: [LAST_AMENDED_DATE]
<!-- 例: Version: 2.1.1 | Ratified: 2025-06-13 | Last Amended: 2025-07-16 -->
