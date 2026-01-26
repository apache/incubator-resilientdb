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

import * as z from 'zod';
import { zodResolver } from '@hookform/resolvers/zod';
import { useForm } from 'react-hook-form';
import { format } from 'date-fns';
import { getUTCdate, timezoneToOffset } from '@webapp/util/formatDate';

interface UseAnnotationFormProps {
  timezone: 'browser' | 'utc';
  value: {
    content?: string;
    timestamp: number;
  };
}

const newAnnotationFormSchema = z.object({
  content: z.string().min(1, { message: 'Required' }),
});

export const useAnnotationForm = ({
  value,
  timezone,
}: UseAnnotationFormProps) => {
  const {
    register,
    handleSubmit,
    formState: { errors },
    setFocus,
  } = useForm({
    resolver: zodResolver(newAnnotationFormSchema),
    defaultValues: {
      content: value?.content,
      timestamp: format(
        getUTCdate(
          new Date(value?.timestamp * 1000),
          timezoneToOffset(timezone)
        ),
        'yyyy-MM-dd HH:mm'
      ),
    },
  });

  return {
    register,
    handleSubmit,
    errors,
    setFocus,
  };
};
