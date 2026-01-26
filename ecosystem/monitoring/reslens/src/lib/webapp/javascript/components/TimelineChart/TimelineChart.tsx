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

/* eslint-disable import/first */
import 'react-dom';
import React from 'react';

import ReactFlot from 'react-flot';
import 'react-flot/flot/jquery.flot.time.min';
import './Selection.plugin';
import 'react-flot/flot/jquery.flot.crosshair.min';
import './TimelineChartPlugin';
import './Tooltip.plugin';
import './Annotations.plugin';
import './ContextMenu.plugin';

interface TimelineChartProps {
  onSelect: (from: string, until: string) => void;
  className: string;
  ['data-testid']?: string;
}

class TimelineChart extends ReactFlot<TimelineChartProps> {
  componentDidMount() {
    this.draw();

    // TODO: use ref
    $(`#${this.props.id}`).bind('plotselected', (event, ranges) => {
      this.props.onSelect(
        Math.round(ranges.xaxis.from / 1000).toString(),
        Math.round(ranges.xaxis.to / 1000).toString()
      );
    });
  }

  componentDidUpdate() {
    this.draw();
  }

  componentWillReceiveProps() {}

  // copied directly from ReactFlot implementation
  // https://github.com/rodrigowirth/react-flot/blob/master/src/ReactFlot.jsx
  render() {
    const style = {
      height: this.props.height || '100px',
      width: this.props.width || '100%',
    };

    return (
      <div
        data-testid={this.props['data-testid']}
        className={this.props.className}
        id={this.props.id}
        style={style}
      />
    );
  }
}

export default TimelineChart;
