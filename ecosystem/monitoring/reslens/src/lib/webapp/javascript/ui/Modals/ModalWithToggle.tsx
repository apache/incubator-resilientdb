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

import React, { SetStateAction, Dispatch, ReactNode } from 'react';
import classnames from 'classnames';
import OutsideClickHandler from 'react-outside-click-handler';
import styles from './ModalWithToggle.module.scss';

export interface ModalWithToggleProps {
  isModalOpen: boolean;
  setModalOpenStatus: Dispatch<SetStateAction<boolean>>;
  customHandleOutsideClick?: (e: MouseEvent) => void;
  toggleText: string;
  headerEl: string | ReactNode;
  leftSideEl: ReactNode;
  rightSideEl: ReactNode;
  footerEl?: ReactNode;
  noDataEl?: ReactNode;
  modalClassName?: string;
  modalHeight?: string;
}

function ModalWithToggle({
  isModalOpen,
  setModalOpenStatus,
  customHandleOutsideClick,
  toggleText,
  headerEl,
  leftSideEl,
  rightSideEl,
  footerEl,
  noDataEl,
  modalClassName,
  modalHeight,
}: ModalWithToggleProps) {
  const handleOutsideClick = () => {
    setModalOpenStatus(false);
  };

  return (
    <div data-testid="modal-with-toggle" className={styles.container}>
      <OutsideClickHandler
        onOutsideClick={customHandleOutsideClick || handleOutsideClick}
      >
        <button
          id="modal-toggler"
          type="button"
          data-testid="toggler"
          className={styles.toggle}
          onClick={() => setModalOpenStatus((v) => !v)}
        >
          {toggleText}
        </button>
        {isModalOpen && (
          <div
            className={classnames(styles.modal, modalClassName)}
            data-testid="modal"
          >
            <div className={styles.modalHeader} data-testid="modal-header">
              {headerEl}
            </div>
            <div className={styles.modalBody} data-testid="modal-body">
              {noDataEl || (
                <>
                  <div
                    className={styles.side}
                    style={{ ...(modalHeight && { height: modalHeight }) }}
                  >
                    {leftSideEl}
                  </div>
                  <div
                    className={styles.side}
                    style={{ ...(modalHeight && { height: modalHeight }) }}
                  >
                    {rightSideEl}
                  </div>
                </>
              )}
            </div>
            <div className={styles.modalFooter} data-testid="modal-footer">
              {footerEl}
            </div>
          </div>
        )}
      </OutsideClickHandler>
    </div>
  );
}

export default ModalWithToggle;
