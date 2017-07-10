/* -*- C -*- */
/* This file is a part of the omobus-mobile project.
 * Copyright (c) 2006 - 2013 ak-obs, Ltd. <info@omobus.net>.
 * Author: Igor Artemov <i_artemov@ak-obs.ru>.
 *
 * This program is commerce software; you can't redistribute it and/or
 * modify it.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <errno.h>

static int _eno_val = 0;

#ifdef __cplusplus
extern "C" {
#endif

int* __cdecl omcore_errno() {
  return &_eno_val;
}

#ifdef __cplusplus
}
#endif
