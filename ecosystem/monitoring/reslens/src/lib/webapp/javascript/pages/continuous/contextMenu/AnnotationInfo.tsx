/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/* eslint-disable react/jsx-props-no-spreading */
import React, { Dispatch, SetStateAction } from 'react';
import { Popover, PopoverBody, PopoverFooter } from '@webapp/ui/Popover';
import Button from '@webapp/ui/Button';
import { Portal } from '@webapp/ui/Portal';
import TextField from '@webapp/ui/Form/TextField';
import { AddAnnotationProps } from './AddAnnotation.menuitem';
import { useAnnotationForm } from './useAnnotationForm';

interface AnnotationInfo {
  /** where to put the popover in the DOM */
  container: AddAnnotationProps['container'];

  /** where to position the popover */
  popoverAnchorPoint: AddAnnotationProps['popoverAnchorPoint'];
  timestamp: AddAnnotationProps['timestamp'];
  timezone: AddAnnotationProps['timezone'];
  value: { content: string; timestamp: number };
  isOpen: boolean;
  onClose: () => void;
  popoverClassname?: string;
}

const AnnotationInfo = ({
  container,
  popoverAnchorPoint,
  value,
  timezone,
  isOpen,
  onClose,
  popoverClassname,
}: AnnotationInfo) => {
  const { register, errors } = useAnnotationForm({ value, timezone });

  return (
    <Portal container={container}>
      <Popover
        anchorPoint={{ x: popoverAnchorPoint.x, y: popoverAnchorPoint.y }}
        isModalOpen={isOpen}
        setModalOpenStatus={onClose as Dispatch<SetStateAction<boolean>>}
        className={popoverClassname}
      >
        <PopoverBody>
          <form id="annotation-form" name="annotation-form">
            <TextField
              {...register('content')}
              label="Description"
              errorMessage={errors.content?.message}
              readOnly
              data-testid="annotation_content_input"
            />
            <TextField
              {...register('timestamp')}
              label="Time"
              type="text"
              readOnly
              data-testid="annotation_timestamp_input"
            />
          </form>
        </PopoverBody>
        <PopoverFooter>
          <Button onClick={onClose} kind="secondary" form="annotation-form">
            Close
          </Button>
        </PopoverFooter>
      </Popover>
    </Portal>
  );
};

export default AnnotationInfo;
