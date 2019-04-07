/**************************************************************************
 * Copyright (C) 2017-2017  Junlon2006
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **************************************************************************
 *
 * Description : uni_pipeline.h
 * Author      : junlon2006@163.com
 * Date        : 2019.04.07
 *
 **************************************************************************/

#ifndef PIPELINE_INC_UNI_PIPELINE_H_
#define PIPELINE_INC_UNI_PIPELINE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "list_head.h"

typedef enum {
  PIPELINE_START,
  PIPELINE_STOP,
  PIPELINE_BREAK
} PipelineEventType;

typedef struct {
  PipelineEventType type;
  void              *content;
} PipelineEvent;

struct PipelineNode;
typedef int (*PipelineAcceptData)(struct PipelineNode *pipeline,
                                  char *buffer, int bytes_len);
typedef int (*PipelineAcceptCtrl)(struct PipelineNode *pipeline,
                                  PipelineEvent *event);

typedef struct PipelineNode {
  struct PipelineNode *self;
  PipelineAcceptData  data;
  PipelineAcceptCtrl  cmd;
  list_head           link;
  list_head           rear_list;
} PipelineNode;

int PipelineNodeInit(PipelineNode *node, PipelineAcceptCtrl cmd,
                     PipelineAcceptData data);
int PipelineConnect(PipelineNode *pre, PipelineNode *rear);
int PipelineDisConnect(PipelineNode *pre, PipelineNode *rear);
int PipelineClear(PipelineNode *node);

#ifdef __cplusplus
}
#endif
#endif  /* PIPELINE_INC_UNI_PIPELINE_H_ */
