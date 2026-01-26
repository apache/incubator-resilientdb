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

import React, { Ref, ChangeEvent } from 'react';
import { DebounceInput } from 'react-debounce-input';
import styles from './Input.module.scss';

interface InputProps {
  testId?: string;
  className?: string;
  type: 'search' | 'text' | 'password' | 'email' | 'number';
  name: string;
  placeholder?: string;
  minLength?: number;
  debounceTimeout?: number;
  onChange: (e: ChangeEvent<HTMLInputElement>) => void;
  value: string | number;
  htmlId?: string;
}

/**
 * @deprecated use TextField instead
 */
const Input = React.forwardRef(
  (
    {
      testId,
      className,
      type,
      name,
      placeholder,
      minLength = 0,
      debounceTimeout = 100,
      onChange,
      value,
      htmlId,
    }: InputProps,
    ref?: Ref<HTMLInputElement>
  ) => {
    return (
      <DebounceInput
        inputRef={ref}
        data-testid={testId}
        className={`${styles.input} ${className || ''}`}
        type={type}
        name={name}
        placeholder={placeholder}
        minLength={minLength}
        debounceTimeout={debounceTimeout}
        onChange={onChange}
        value={value}
        id={htmlId}
      />
    );
  }
);

export default Input;
