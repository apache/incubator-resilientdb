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

import React, { ReactNode } from 'react';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
import type { IconDefinition } from '@fortawesome/fontawesome-common-types';
import cx from 'classnames';
import styles from './Button.module.scss';

export interface ButtonProps {
  kind?: 'default' | 'primary' | 'secondary' | 'danger' | 'outline' | 'float';
  // kind?: 'default' | 'primary' | 'secondary' | 'danger' | 'float' | 'outline';
  /** Whether the button is disabled or not */
  disabled?: boolean;
  icon?: IconDefinition;
  iconNode?: ReactNode;

  children?: React.ReactNode;

  /** Buttons are grouped so that only the first and last have clear limits */
  grouped?: boolean;

  onClick?: (event: React.MouseEvent<HTMLButtonElement>) => void;

  // TODO
  // for the full list use refer to https://developer.mozilla.org/en-US/docs/Web/HTML/Element/input
  type?: 'button' | 'submit';
  ['data-testid']?: string;

  ['aria-label']?: string;

  className?: string;

  id?: string;
  form?: React.ButtonHTMLAttributes<HTMLButtonElement>['form'];

  /** disable a box around it */
  noBox?: boolean;

  /** ONLY use this if within a modal (https://stackoverflow.com/a/71848275 and https://citizensadvice.github.io/react-dialogs/modal/auto_focus/index.html) */
  autoFocus?: React.ButtonHTMLAttributes<HTMLButtonElement>['autoFocus'];
}

const Button = React.forwardRef(function Button(
  {
    disabled = false,
    kind = 'default',
    type = 'button',
    icon,
    iconNode,
    children,
    grouped,
    onClick,
    id,
    className,
    form,
    noBox,
    autoFocus,
    ...props
  }: ButtonProps,
  ref: React.LegacyRef<HTMLButtonElement>
) {
  return (
    <button
      // needed for tooltip
      // eslint-disable-next-line react/jsx-props-no-spreading
      {...props}
      id={id}
      ref={ref}
      type={type}
      data-testid={props['data-testid']}
      disabled={disabled}
      onClick={onClick}
      form={form}
      autoFocus={autoFocus} // eslint-disable-line jsx-a11y/no-autofocus
      aria-label={props['aria-label']}
      className={cx(
        styles.button,
        grouped ? styles.grouped : '',
        getKindStyles(kind),
        className,
        noBox && styles.noBox,
        iconNode && styles.customIcon,
        !icon && !iconNode && styles.noIcon
      )}
    >
      {icon ? (
        <FontAwesomeIcon
          icon={icon}
          className={children ? styles.iconWithText : ''}
        />
      ) : null}
      {iconNode}
      {children}
    </button>
  );
});

function getKindStyles(kind: ButtonProps['kind']) {
  switch (kind) {
    case 'default': {
      return styles.default;
    }

    case 'primary': {
      return styles.primary;
    }

    case 'secondary': {
      return styles.secondary;
    }

    case 'danger': {
      return styles.danger;
    }

    case 'outline': {
      return styles.outline;
    }

    case 'float': {
      return styles.float;
    }

    default: {
      throw Error(`Unsupported kind ${kind}`);
    }
  }
}

export default Button;
