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

import React from 'react';
import FlameGraphRenderer from './FlameGraph/FlameGraphRenderer';
// Using external CSS instead of local SASS to avoid import issues
// import './sass/flamegraph.scss';

const overrideProps = {
  //  showPyroscopeLogo: !process.env.PYROSCOPE_HIDE_LOGO as any, // this is injected by webpack
  showPyroscopeLogo: false,
};

export type FlamegraphRendererProps = Omit<
  FlameGraphRenderer['props'],
  'showPyroscopeLogo'
>;

// Module augmentation so that typescript sees our 'custom' element
declare global {
  // eslint-disable-next-line @typescript-eslint/no-namespace
  namespace JSX {
    interface IntrinsicElements {
      'pyro-flamegraph': React.DetailedHTMLProps<
        React.HTMLAttributes<HTMLElement>,
        HTMLElement
      >;
    }
  }
}

// TODO: type props
export const FlamegraphRenderer = (props: FlamegraphRendererProps) => {
  // Although 'flamegraph' is not a valid HTML element
  // It's used to scope css without affecting specificity
  // For more info see flamegraph.scss
  return (
    <pyro-flamegraph is="span">
      {/* eslint-disable-next-line react/jsx-props-no-spreading */}
      <FlameGraphRenderer {...props} {...overrideProps} />
    </pyro-flamegraph>
  );
};
