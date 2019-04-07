/**************************************************************************
 * Copyright (C) 2018-2019  Junlon2006
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
 * Description : uni_pipeline.c
 * Author      : junlon2006@163.com
 * Date        : 2019.04.07
 *
 **************************************************************************/
#include "uni_pipeline.h"

#include "uni_log.h"
#include "list_head.h"
#include <stdio.h>

#define PIPELINE_TAG  "pipeline"

int PipelineNodeInit(PipelineNode *node, PipelineAcceptCtrl cb_cmd,
                     PipelineAcceptData cb_data) {
  if (NULL == node) {
    LOGE(PIPELINE_TAG, "param invalid. node=%p", node);
    return -1;
  }
  node->self = node;
  node->data = cb_data;
  node->cmd = cb_cmd;
  list_init(&node->link);
  list_init(&node->rear_list);
  return 0;
}

int PipelineConnect(PipelineNode *pre, PipelineNode *rear) {
  if (NULL == pre || NULL == rear) {
    LOGE(PIPELINE_TAG, "param invalid. pre=%p, rear=%p", pre, rear);
    return -1;
  }
  list_add(&rear->link, &pre->rear_list);
  return 0;
}

int PipelineDisConnect(PipelineNode *pre, PipelineNode *rear) {
  PipelineNode *p;
  if (NULL == pre || NULL == rear) {
    LOGE(PIPELINE_TAG, "param invalid. pre=%p, rear=%p", pre, rear);
    return -1;
  }
  list_for_each_entry(p, &pre->rear_list, PipelineNode, link) {
    if (p == rear) {
      list_del(&p->link);
      break;
    }
  }
  return 0;
}

int PipelineClear(PipelineNode *pipeline) {
  PipelineNode *p, *n;
  if (NULL == pipeline) {
    LOGE(PIPELINE_TAG, "param invalid. node=%p", pipeline);
  }
  list_for_each_entry_safe(p, n, &pipeline->rear_list, PipelineNode, link) {
    list_del(&p->link);
  }
  return 0;
}
