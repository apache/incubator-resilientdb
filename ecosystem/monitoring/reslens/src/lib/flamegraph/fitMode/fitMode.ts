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

export const TailMode = 'TAIL';
export const HeadMode = 'HEAD';

export type FitModes = typeof TailMode | typeof HeadMode;

const margin = 3;

/**
 * Returns a text and margin left used to write text into a canvas rectangle
 *
 * @param {FitModes} mode -
 * @param {number} charSize - Size in pixels of an individual character. Assumes it's a monospace font.
 * @param {number} rectWidth - Width in pixels of the rectangle
 * @param {string} fullText - The text that will be first tried.
 * @param {string} shortText - The text that willbe used when fullText can't fit. It's normally a substring of the original text.
 */

interface fitToCanvasRectProps {
  mode: FitModes;

  /** charSize - Size in pixels of an individual character. Assumes it's a monospace font. */
  charSize: number;

  /** Width in pixels of the rectangle */
  rectWidth: number;

  /** The text that will be first tried to fit */
  fullText: string;

  /** The text that willbe used when fullText can't fit. It's normally a substring of the original text. */
  shortText: string;
}

export function fitToCanvasRect({
  mode,
  charSize,
  rectWidth,
  fullText,
  shortText,
}: fitToCanvasRectProps) {
  switch (mode) {
    case TailMode:
      // Case 1:
      // content fits rectangle width
      // | rectangle |
      // | text |
      if (charSize * fullText.length <= rectWidth) {
        // assume it's a monospaced font
        return {
          mode,
          text: fullText,
          marginLeft: margin,
        };
      }

      // assume it's a monospaced font
      // if not the case, use
      // ctx.measureText(shortName).width
      const shortTextWidth = charSize * shortText.length;

      // Case 2:
      // short text fits rectangle width
      // | rectangle |
      // | long_text_text |
      // | shorttext |
      if (shortTextWidth <= rectWidth) {
        // assume it's a monospaced font
        return {
          mode,
          text: shortText,
          marginLeft: margin,
        };
      }

      // Case 3:
      // short text is bigger than rectangle width
      // add a negative margin left
      // so that the 'tail' of the string is visible
      //     | rectangle |
      // | my_short_text |
      return {
        mode,
        text: shortText,
        marginLeft: -(shortTextWidth - rectWidth + margin),
      };

    // Case 3:
    // Normal
    case HeadMode:
    default:
      return {
        mode,
        text: fullText,
        marginLeft: margin,
      };
  }
}

/**
 * Returns an inline style in React format
 * used to fit the content into a table cell
 * or an empty object if not applicable.
 * @param {FitModes} mode - The mode
 */
export function fitIntoTableCell(mode: FitModes): React.CSSProperties {
  switch (mode) {
    case TailMode:
      return {
        // prints from right to left
        direction: 'rtl',
        overflow: 'hidden',
        textOverflow: 'ellipsis',
      };

    case HeadMode:
    default:
      return {
        overflow: 'hidden',
        textOverflow: 'ellipsis',
      };
  }
}
