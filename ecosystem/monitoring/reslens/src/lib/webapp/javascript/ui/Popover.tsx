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

import React, {
  useRef,
  useState,
  useLayoutEffect,
  SetStateAction,
  Dispatch,
  ReactNode,
} from 'react';
import classnames from 'classnames';
import OutsideClickHandler from 'react-outside-click-handler';
import styles from './Popover.module.scss';

export interface PopoverProps {
  isModalOpen: boolean;
  setModalOpenStatus: Dispatch<SetStateAction<boolean>>;
  children: ReactNode;
  className?: string;

  /** where to position the popover on the page */
  anchorPoint: {
    x: number | string;
    y: number;
  };
}

export function Popover({
  isModalOpen,
  setModalOpenStatus,
  className,
  children,
  anchorPoint,
}: PopoverProps) {
  const popoverRef = useRef<HTMLDivElement>(null);
  const [popoverPosition, setPopoverPosition] = useState<React.CSSProperties>({
    display: 'hidden',
  });

  useLayoutEffect(() => {
    if (isModalOpen && popoverRef.current) {
      const pos = getPopoverPosition(
        popoverRef.current.clientWidth,
        window.innerWidth,
        anchorPoint
      );
      setPopoverPosition(pos);
    }
  }, [isModalOpen]);

  return (
    <OutsideClickHandler onOutsideClick={() => setModalOpenStatus(false)}>
      <div
        className={styles.container}
        style={popoverPosition}
        ref={popoverRef}
      >
        {isModalOpen && (
          <div className={classnames(styles.popover, className)}>
            {children}
          </div>
        )}
      </div>
    </OutsideClickHandler>
  );
}

function getPopoverPosition(
  popoverWidth: number,
  windowWidth: number,
  anchorPoint: PopoverProps['anchorPoint']
) {
  // Give some room between popover end and the window edge
  const marginToWindowEdge = 30;
  const defaultProps = {
    top: `${anchorPoint.y}px`,
    position: 'absolute' as const,
  };

  if (typeof anchorPoint.x === 'string') {
    return {
      ...defaultProps,
      left: anchorPoint.x,
    };
  }

  if (anchorPoint.x + popoverWidth + marginToWindowEdge >= windowWidth) {
    // position to the left
    return {
      ...defaultProps,
      left: `${anchorPoint.x - popoverWidth}px`,
    };
  }

  // position to the right
  return {
    ...defaultProps,
    left: `${anchorPoint.x}px`,
  };
}
interface PopoverMemberProps {
  children: ReactNode;
  className?: string;
}

export function PopoverHeader({ children, className }: PopoverMemberProps) {
  return <div className={classnames(styles.header, className)}>{children}</div>;
}

export function PopoverBody({ children, className }: PopoverMemberProps) {
  return <div className={classnames(styles.body, className)}>{children}</div>;
}

export function PopoverFooter({ children, className }: PopoverMemberProps) {
  return <div className={classnames(styles.footer, className)}>{children}</div>;
}
