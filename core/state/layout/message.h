/* Imhotep - the city message records (chunk 77: 1000 records of 0x30 bytes) and their fields.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Evidence: docs/formats/save.md §5 "Messages" and the message post 0x004E0CF0
 * (core/city/month_message.c). Conventions: docs/naming.md. */
#ifndef IMHOTEP_STATE_LAYOUT_MESSAGE_H
#define IMHOTEP_STATE_LAYOUT_MESSAGE_H

#include <stdint.h>

/* @bin 0x004E0CF0, 0x004E1750, 0x004E1640: chunk 77 */
#define MESSAGE_BASE 0x00E95228u
#define MESSAGE_REC_SIZE 0x30u
#define MESSAGE_COUNT 1000
#define MESSAGE_ADDR(k) (MESSAGE_BASE + MESSAGE_REC_SIZE * (uint32_t) (k))

/* @bin 0x004E0D67 the post writes the record */
#define MSG_PARAM_A 0x00u    /* i32: first parameter */
#define MSG_PARAM_B 0x04u    /* i32: second parameter */
#define MSG_YEAR 0x08u       /* i16: G_YEAR at the post */
#define MSG_ID 0x0Au         /* i16: message id (0: free record) */
#define MSG_SEQUENCE 0x0Cu   /* i16: G_MESSAGE_SEQUENCE at the post */
#define MSG_READ 0x0Eu       /* u8: 1 once shown */
#define MSG_MONTH 0x0Fu      /* u8: G_MONTH at the post */
#define MSG_DATA 0x10u       /* u8[30]: the post's optional block; without one, three i16 -1 */
#define MSG_DATA_SIZE 30

/* @bin 0x004E0CF0 the message types (G_MESSAGE_TYPES): one entry per message id */
#define MSGTYPE_SIZE 0x50u
#define MSGTYPE_U0A 0x0Au    /* i16: 1 or 7 set G_UE951D8 and G_REDRAW */
#define MSGTYPE_U40 0x40u    /* i32: 1 selects 7, else 6, for 0x0047D350 */
#define MSGTYPE_VIDEO 0x44u  /* u32: the video's name, offset in G_MM_TEXT (checked by 0x004CE3F0) */

/* @bin 0x004E2970 a notice (G_NOTICES) */
#define NOTICE_SIZE 0x78u
#define NOTICE_KIND 0x00u    /* i16: the post's kind */
#define NOTICE_ACTIVE 0x06u  /* i16: 0 free */
#define NOTICE_LENGTH 0x08u  /* i16: the text's length */
#define NOTICE_WIDTH 0x0Au   /* i16: 200, 300, 400 or 460 by the text's width */
#define NOTICE_TIME 0x0Cu    /* i32: G_FRAME_CLOCK at the post */
#define NOTICE_TEXT 0x12u    /* char[0x64] */
#define NOTICE_TEXT_SIZE 0x64

#endif
