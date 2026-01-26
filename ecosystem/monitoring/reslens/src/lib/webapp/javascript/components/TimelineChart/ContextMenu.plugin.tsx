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
import * as ReactDOM from 'react-dom';
import { randomId } from '@webapp/util/randomId';

// Pre calculated once
// TODO(eh-am): does this work with multiple contextMenus?
const WRAPPER_ID = randomId('context_menu');

export interface ContextMenuProps {
  click: {
    /** The X position in the window where the click originated */
    pageX: number;
    /** The Y position in the window where the click originated */
    pageY: number;
  };
  timestamp: number;
  containerEl: HTMLElement;
}

(function ($: JQueryStatic) {
  function init(plot: jquery.flot.plot & jquery.flot.plotOptions) {
    const placeholder = plot.getPlaceholder();

    function onClick(
      event: unknown,
      pos: { x: number; pageX: number; pageY: number }
    ) {
      const options: jquery.flot.plotOptions & {
        ContextMenu?: React.FC<ContextMenuProps>;
      } = plot.getOptions();
      const container = inject($);
      const containerEl = container?.[0];

      // unmount any previous menus
      ReactDOM.unmountComponentAtNode(containerEl);

      const ContextMenu = options?.ContextMenu;

      if (ContextMenu && containerEl) {
        ReactDOM.render(
          <ContextMenu
            click={{ ...pos }}
            containerEl={containerEl}
            timestamp={Math.round(pos.x / 1000)}
          />,
          containerEl
        );
      }
    }

    // Register events and shutdown
    // It's important to bind/unbind to the SAME element
    // Since a plugin may be register/unregistered multiple times due to react re-rendering
    plot.hooks!.bindEvents!.push(function () {
      placeholder.bind('plotclick', onClick);
    });

    plot.hooks!.shutdown!.push(function () {
      placeholder.unbind('plotclick', onClick);

      const container = inject($);

      ReactDOM.unmountComponentAtNode(container?.[0]);
    });
  }

  $.plot.plugins.push({
    init,
    options: {},
    name: 'context_menu',
    version: '1.0',
  });
})(jQuery);

function inject($: JQueryStatic) {
  const alreadyInitialized = $(`#${WRAPPER_ID}`).length > 0;

  if (alreadyInitialized) {
    return $(`#${WRAPPER_ID}`);
  }

  const body = $('body');
  return $(`<div id="${WRAPPER_ID}" />`).appendTo(body);
}
