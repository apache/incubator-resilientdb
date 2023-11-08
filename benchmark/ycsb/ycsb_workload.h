//
//  core_workload.h
//  YCSB-C
//
//  Created by Jinglei Ren on 12/9/14.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#pragma once

#include "core/core_workload.h"

namespace resdb {
namespace benchmark {

class YCSBWorkload : public ycsbc::CoreWorkload {
 public:
  /// Initialize the scenario.
  /// Called once, in the main client thread, before any operations are started.
  ///
  virtual void Init(const utils::Properties &p);
};


}
}