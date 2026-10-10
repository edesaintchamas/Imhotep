/* Imhotep - named globals: the city messages (records: layout/message.h) and notices.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md. */
#ifndef IMHOTEP_STATE_LAYOUT_G_MESSAGE_H
#define IMHOTEP_STATE_LAYOUT_G_MESSAGE_H

/* @bin 0x004E0CF0 the post, 0x004E1640 the sort */
#define G_MESSAGE_SEQUENCE 0x00E951F0u /* i32: next MSG_SEQUENCE (chunk 78) */
#define G_MESSAGE_TOTAL 0x00E951ECu    /* i32: messages in the list */
#define G_MESSAGE_SLOT 0x00E951E8u     /* i32: the record last posted */
#define G_MESSAGE_CURRENT 0x00E951E4u  /* i32: the record shown, or -1 */
#define G_MESSAGE_SHOWN_ID 0x00E951DCu /* i32: the message id the window opens on */
#define G_UE951D8 0x00E951D8u          /* i32 := 1 by a post of a message type whose MSGTYPE_U0A is 1 or 7 */
#define G_MESSAGE_TEMP 0x00E951F8u     /* 0x30 bytes: the sort's swap buffer */
#define G_MESSAGE_SCROLL_ROWS 0x00E9511Cu /* i32: scrollable rows of the list (messages - 10) */
#define G_MESSAGE_SCROLL 0x00E95120u   /* i32: the first message shown in the list */
#define G_MESSAGE_PENDING 0x00E95080u  /* i32[20]: MSG_SEQUENCE of messages waiting for the city window */
#define G_MESSAGE_PENDING_COUNT 20
#define G_MESSAGE_POPUP 0x00E950DCu    /* i32: the post opens a pop-up (forced by callers, or 0x004E0F90) */
#define G_UE950E0 0x00E950E0u          /* i32 := 1 by every post */
#define G_MESSAGE_SETTINGS 0x00E38EF0u /* @bin 0x004E0F90 u32: one bit per filtered message kind */
#define G_MESSAGE_ADVISOR 0x00D3A6E8u  /* @bin 0x004E14B0 i32: the advisor kind of a message id */
/* @bin 0x004E0E6A the message window's parameters, from the record */
#define G_MESSAGE_WINDOW_YEAR 0x00D3B178u
#define G_MESSAGE_WINDOW_MONTH 0x00D3B174u
#define G_MESSAGE_WINDOW_PARAM_A 0x00D3B170u
#define G_MESSAGE_WINDOW_PARAM_B 0x00D3B16Cu
/* @bin 0x004E0CF0 the message types: Pharaoh_MM.eng's index (docs/formats/state.md), one entry of
 * MSGTYPE_SIZE bytes per message id; fields in layout/message.h */
#define G_MESSAGE_TYPES 0x00E209B0u

/* @bin 0x004E2970 the notices: G_NOTICE_COUNT records of NOTICE_SIZE bytes (layout/message.h) */
#define G_NOTICES 0x00E94E28u /* @bin 0x00E94E2E is NOTICE_ACTIVE of the first */
#define G_NOTICE_COUNT 5 /* @bin 0x00E94E2E .. 0x00E95086: NOTICE_ACTIVE of each */
#define G_NOTICES_ON 0x00E38EC1u       /* u8: notices shown */
#define G_NOTICE_POSTED 0x00E94E20u    /* i32 := 1 by a post */
#define G_UF53F34 0x00F53F34u          /* i32 per notice, cleared by a post */

/* Package P3 (phase 10.3) */
#define G_MESSAGE_DELAYS 0x00E95124u /* @bin 0x004E1850 i32[20]: per-category message delays, monthly -1 (save.md §3 chunk 83) */
#define G_MESSAGE_COUNTERS 0x00E95174u /* @bin 0x00468B70 i32[20]: per-category message counters (save.md §3 chunk 82) */
#define G_POPULATION_MILESTONES 0x00E951C5u /* @bin 0x00458649 u8[9]: the population messages posted (500 .. 25000) */

#endif
