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

// we only need this import for its side effects (ie importing flot types)
// eslint-disable-next-line import/no-unresolved
import 'flot';

// @types/flot only exposes plotOptions
// but flot in fact exposes more parameters to us
// https://github.com/flot/flot/blob/370cf6ee85de0e0fcae5bf084e0986cda343e75b/source/jquery.flot.js#L361
type plotInitPluginParams = jquery.flot.plot & jquery.flot.plotOptions;
declare global {
  declare namespace jquery.flot {
    interface plugin {
      init(plot: plotInitPluginParams): void;
    }

    interface plot {
      p2c(point: point): canvasPoint;
    }
  }
}
