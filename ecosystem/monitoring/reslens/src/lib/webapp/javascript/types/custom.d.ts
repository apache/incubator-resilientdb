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

/* eslint-disable max-classes-per-file */

// Got from https://github.com/rodrigowirth/react-flot/blob/master/src/ReactFlot.d.ts
declare module 'react-flot' {
  interface ReactFlotProps {
    id: string;
    data: ShamefulAny[];
    options: object;
    height?: string;
    width?: string;
  }

  class ReactFlot<CustomProps> extends React.Component<
    ReactFlotProps & CustomProps,
    ShamefulAny
  > {
    componentDidMount(): void;

    // componentWillReceiveProps(nextProps: any): void;

    draw(event?: ShamefulAny, data?: ShamefulAny): void;

    render(): ShamefulAny;
  }
  export = ReactFlot;
}

// From https://github.com/chantastic/react-svg-spinner/blob/master/index.d.ts
declare module 'react-svg-spinner' {
  import React from 'react';

  type SpinnerProps = {
    size?: string;
    width?: string;
    height?: string;
    color?: string;
    thickness?: number;
    gap?: number;
    speed?: 'fast' | 'slow' | 'default';
  };

  // eslint-disable-next-line react/prefer-stateless-function
  class Spinner extends React.Component<SpinnerProps, ShamefulAny> {}

  export default Spinner;
}
