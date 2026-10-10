/* Imhotep - pending names of package p7 (docs/naming.md "Pending names").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Globals this package names that belong to another package's area header (or to an area header it
 * does not own): defined here, one per line with its evidence, until the lead merges them into the
 * owning header and empties this file. Only this package edits this file. @bin evidence: on each
 * name's line. */
#ifndef IMHOTEP_STATE_LAYOUT_PENDING_P7_FRAME_H
#define IMHOTEP_STATE_LAYOUT_PENDING_P7_FRAME_H

#define NOTICE_X 0x02u /* @bin 0x004E2720 i16: the notice's x centre (0x004E2AB0 sets it) [message.h] */
#define NOTICE_Y 0x04u /* @bin 0x004E2720 i16: its y [message.h] */
#define EMPOBJ_X 0x04u  /* @bin 0x00407C10 i16: the object's x on the empire map [with EMPOBJ_IN_USE] */
#define EMPOBJ_X2 0x14u /* @bin 0x00407C10 i16: a second x (+2 its y) the editor moves when G_EMPIRE_ALT_POSITIONS (assumed its name's) [with EMPOBJ_IN_USE] */
#define G_EMPIRE_ALT_POSITIONS 0x00784CB4u /* @bin 0x00407C10 i32: the empire editor moves the objects' second position [g_ui_main.h] */
#define G_INFO_ADVISORS 0x00EADCE8u /* @bin 0x00515BF0 i32[15]: the advisor buttons the info panel lists (0x00515130) [g_ui_info.h] */
#define G_ADVISOR_BUTTONS 0x005F78F0u /* @bin 0x00515BF0 one button table per advisor page (4 bytes each) [g_ui_main.h] */

#endif
