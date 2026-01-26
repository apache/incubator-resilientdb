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
import { render, screen } from '@testing-library/react';

import ModalWithToggle, { ModalWithToggleProps } from './ModalWithToggle';

const defaultProps = {
  isModalOpen: false,
  setModalOpenStatus: jest.fn(),
  toggleText: 'toggle-test-text',
  headerEl: <div />,
  leftSideEl: <div />,
  rightSideEl: <div />,
  footerEl: <div />,
};

describe('Component: ModalWithToggle', () => {
  const renderComponent = (props: ModalWithToggleProps) => {
    render(<ModalWithToggle {...props} />);
  };

  describe('structure', () => {
    it('should display toggler button', () => {
      renderComponent(defaultProps);

      expect(screen.getByTestId('toggler')).toHaveTextContent(
        defaultProps.toggleText
      );
    });

    it('should display modal after toggler click', () => {
      renderComponent({ ...defaultProps, isModalOpen: true });

      expect(screen.getByTestId('modal')).toBeInTheDocument();
      expect(screen.getByTestId('modal-header')).toBeInTheDocument();
      expect(screen.getByTestId('modal-body')).toBeInTheDocument();
      expect(screen.getByTestId('modal-footer')).toBeInTheDocument();
    });
  });

  describe('optional props', () => {
    it('props: noDataEl', () => {
      renderComponent({
        ...defaultProps,
        isModalOpen: true,
        noDataEl: <div data-testid="no-data" />,
      });
      expect(screen.getByTestId('no-data')).toBeInTheDocument();
    });

    it('props: modalClassName', () => {
      renderComponent({
        ...defaultProps,
        isModalOpen: true,
        modalClassName: 'test-class-name',
      });
      expect(screen.getByTestId('modal').getAttribute('class')).toContain(
        'test-class-name'
      );
    });

    it('props: modalHeight', () => {
      renderComponent({
        ...defaultProps,
        modalHeight: '100px',
        isModalOpen: true,
      });
      expect(
        screen
          .getByTestId('modal')
          .querySelector('.side')
          ?.getAttribute('style')
      ).toContain('100px');
    });
  });
});
