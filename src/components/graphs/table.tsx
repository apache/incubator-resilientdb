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
*
*/

import * as React from "react";
import {
  type ColumnDef,
  type ColumnFiltersState,
  type SortingState,
  type VisibilityState,
  flexRender,
  getCoreRowModel,
  getFilteredRowModel,
  getPaginationRowModel,
  getSortedRowModel,
  useReactTable,
} from "@tanstack/react-table";
import { ArrowUpDown, ChevronDown, DotIcon } from "lucide-react";

import { Button } from "@/components/ui/button";
import {
  DropdownMenu,
  DropdownMenuCheckboxItem,
  DropdownMenuContent,
  DropdownMenuTrigger,
} from "@/components/ui/dropdown-menu";
import { Input } from "@/components/ui/input";
import {
  Table,
  TableBody,
  TableCell,
  TableHead,
  TableHeader,
  TableRow,
} from "@/components/ui/table";
import { Skeleton } from "../ui/skeleton";
import { Badge } from "../ui/badge";
import { Link } from "react-router-dom";
import { Select, SelectContent, SelectItem, SelectTrigger } from "../ui/select";
import { middlewareApi } from "@/lib/api";

export interface Transaction {
  cmd: string;
  key: string;
  value?: string;
}

export interface Block {
  id: number;
  number: string;
  transactions: Transaction[];
  size: number;
  createdAt: string;
}

const columns: ColumnDef<Block>[] = [
  {
    accessorKey: "number",
    header: ({ column }) => {
      return (
        <Button
          variant="ghost"
          onClick={() => column.toggleSorting(column.getIsSorted() === "asc")}
        >
          Block #
          <ArrowUpDown className="ml-2 h-4 w-4" />
        </Button>
      );
    },
    cell: ({ row }) => (
      <Link to="#" className="flex align-middle">
        {" "}
        <DotIcon className="h-16 w-16 text-blue-300" />
        <span className="my-auto">{row.getValue("number")}</span>
      </Link>
    ),
  },
  {
    accessorKey: "size",
    header: ({ column }) => {
      return (
        <Button
          variant="ghost"
          onClick={() => column.toggleSorting(column.getIsSorted() === "asc")}
        >
          Size
          <ArrowUpDown className="ml-2 h-4 w-4" />
        </Button>
      );
    },
    cell: ({ row }) => <div>{`${row.getValue("size")} Bytes`}</div>,
  },
  {
    accessorKey: "transactions",
    header: "Command",
    cell: ({ row }) => {
      const transactions = row.getValue(
        "transactions"
      ) as Block["transactions"];
      const command =
        transactions && transactions.length > 0
          ? transactions[0]?.cmd
          : "No Command";
      return <Badge variant="outline">{command}</Badge>;
    },
  },
  {
    accessorKey: "createdAt",
    header: ({ column }) => {
      return (
        <Button
          variant="ghost"
          onClick={() => column.toggleSorting(column.getIsSorted() === "asc")}
        >
          Created At
          <ArrowUpDown className="ml-2 h-4 w-4" />
        </Button>
      );
    },
    cell: ({ row }) => (
      <div>{new Date(row.getValue("createdAt")).toLocaleString()}</div>
    ),
  },
];

export function BlockchainTable({ total }: { total: number }) {
  const [loading, setLoading] = React.useState(false);
  const [data, setData] = React.useState<Block[]>([]);
  const [pageIndex, setPageIndex] = React.useState(0);
  const [pageSize, setPageSize] = React.useState(10);

  // Fetch blocks data
  const fetchBlocksData = React.useCallback(async () => {
    console.log("Page Idx", pageIndex);
    try {
      setLoading(true);

      const start = pageIndex * pageSize + 1;
      const end = Math.min(total, start + pageSize - 1);

      const response = await middlewareApi.get(`/explorer/getBlocks`, {
        params: { start, end },
      });
      console.log("Response is: ", response?.data);
      setData(response?.data as Block[]);
      setLoading(false);
    } catch (error) {
      console.error(error);
      setLoading(false);
    }
  }, [pageIndex, pageSize, total]);

  // Run fetchBlocksData when pageIndex or pageSize changes
  React.useEffect(() => {
    fetchBlocksData();
  }, [fetchBlocksData]); // Direct dependencies

  const totalPages = Math.ceil(total / pageSize);

  const handlePageChange = (newPageIndex: number) => {
    if (newPageIndex >= 0 && newPageIndex < totalPages) {
      console.log("Changing page from", pageIndex, "to", newPageIndex);
      setPageIndex(newPageIndex);
    }
  };

  const visiblePages = 5;
  const startPage = Math.max(
    0,
    Math.min(
      pageIndex - Math.floor(visiblePages / 2),
      totalPages - visiblePages
    )
  );
  const endPage = Math.min(startPage + visiblePages, totalPages);

  const [sorting, setSorting] = React.useState<SortingState>([]);
  const [columnFilters, setColumnFilters] = React.useState<ColumnFiltersState>(
    []
  );
  const [columnVisibility, setColumnVisibility] =
    React.useState<VisibilityState>({});

  const table = useReactTable({
    data,
    columns,
    manualPagination: true,
    onSortingChange: setSorting,
    onColumnFiltersChange: setColumnFilters,
    getCoreRowModel: getCoreRowModel(),
    getPaginationRowModel: getPaginationRowModel(),
    getSortedRowModel: getSortedRowModel(),
    getFilteredRowModel: getFilteredRowModel(),
    onColumnVisibilityChange: setColumnVisibility,
    state: {
      sorting,
      columnFilters,
      columnVisibility,
      pagination: { pageIndex, pageSize },
    },
    pageCount: totalPages,
  });

  if (loading) {
    return (
      <div className="w-full">
        <div className="flex items-center py-4">
          <Skeleton className="h-8 w-[250px]" />
          <Skeleton className="ml-auto h-8 w-[100px]" />
        </div>
        <div className="rounded-md border">
          <Table>
            <TableHeader>
              <TableRow>
                <TableHead>
                  <Skeleton className="h-4 w-[100px]" />
                </TableHead>
                <TableHead>
                  <Skeleton className="h-4 w-[100px]" />
                </TableHead>
                <TableHead>
                  <Skeleton className="h-4 w-[100px]" />
                </TableHead>
                <TableHead>
                  <Skeleton className="h-4 w-[100px]" />
                </TableHead>
              </TableRow>
            </TableHeader>
            <TableBody>
              {[...Array(10)].map((_, index) => (
                <TableRow key={index}>
                  <TableCell>
                    <Skeleton className="h-4 w-[50px]" />
                  </TableCell>
                  <TableCell>
                    <Skeleton className="h-4 w-[100px]" />
                  </TableCell>
                  <TableCell>
                    <Skeleton className="h-4 w-[50px]" />
                  </TableCell>
                  <TableCell>
                    <Skeleton className="h-4 w-[150px]" />
                  </TableCell>
                </TableRow>
              ))}
            </TableBody>
          </Table>
        </div>
        <div className="flex items-center justify-end space-x-2 py-4">
          <Skeleton className="h-8 w-[80px]" />
          <Skeleton className="h-8 w-[80px]" />
        </div>
      </div>
    );
  } else {
    return (
      <div className="w-full">
        <div className="flex items-center py-4">
          <Input
            placeholder="Filter blocks..."
            value={
              (table.getColumn("number")?.getFilterValue() as string) ?? ""
            }
            onChange={(event) =>
              table.getColumn("number")?.setFilterValue(event.target.value)
            }
            className="max-w-sm"
          />
          <DropdownMenu>
            <DropdownMenuTrigger asChild>
              <Button variant="outline" className="ml-auto">
                Columns <ChevronDown className="ml-2 h-4 w-4" />
              </Button>
            </DropdownMenuTrigger>
            <Select
              value={String(pageSize)}
              onValueChange={(value) => {
                setPageSize(Number(value));
                setPageIndex(0);
              }}
            >
              <SelectTrigger className="w-[150px]">
                <span>Page Size {pageSize}</span>
              </SelectTrigger>
              <SelectContent>
                {[5, 10, 20, 50].map((size) => (
                  <SelectItem key={size} value={String(size)}>
                    Page Size {size}
                  </SelectItem>
                ))}
              </SelectContent>
            </Select>
            <Button
              variant="outline"
              className="ml-4"
              onClick={fetchBlocksData}
            >
              Refresh
            </Button>

            <DropdownMenuContent align="end">
              {table
                .getAllColumns()
                .filter((column) => column.getCanHide())
                .map((column) => {
                  return (
                    <DropdownMenuCheckboxItem
                      key={column.id}
                      className="capitalize"
                      checked={column.getIsVisible()}
                      onCheckedChange={(value) =>
                        column.toggleVisibility(!!value)
                      }
                    >
                      {column.id}
                    </DropdownMenuCheckboxItem>
                  );
                })}
            </DropdownMenuContent>
          </DropdownMenu>
        </div>
        <div className="rounded-md border">
          <Table>
            <TableHeader>
              {table.getHeaderGroups().map((headerGroup) => (
                <TableRow key={headerGroup.id}>
                  {headerGroup.headers.map((header) => {
                    return (
                      <TableHead key={header.id}>
                        {header.isPlaceholder
                          ? null
                          : flexRender(
                              header.column.columnDef.header,
                              header.getContext()
                            )}
                      </TableHead>
                    );
                  })}
                </TableRow>
              ))}
            </TableHeader>
            <TableBody>
              {table.getRowModel().rows?.length ? (
                table.getRowModel().rows.map((row) => (
                  <TableRow
                    key={row.id}
                    data-state={row.getIsSelected() && "selected"}
                  >
                    {row.getVisibleCells().map((cell) => (
                      <TableCell key={cell.id}>
                        {flexRender(
                          cell.column.columnDef.cell,
                          cell.getContext()
                        )}
                      </TableCell>
                    ))}
                  </TableRow>
                ))
              ) : (
                <TableRow>
                  <TableCell
                    colSpan={columns.length}
                    className="h-24 text-center"
                  >
                    No results...
                  </TableCell>
                </TableRow>
              )}
            </TableBody>
          </Table>
        </div>
        <div className="flex items-center justify-end space-x-2 py-4">
          {/* Go to Start Button */}
          <Button
            variant="outline"
            size="sm"
            onClick={() => handlePageChange(0)}
            disabled={pageIndex === 0}
          >
            Go to Start
          </Button>

          {/* Previous Button */}
          <Button
            variant="outline"
            size="sm"
            onClick={() => handlePageChange(pageIndex - 1)}
            disabled={pageIndex === 0}
          >
            Previous
          </Button>

          {/* Page Numbers */}
          {Array.from(
            { length: endPage - startPage },
            (_, i) => startPage + i
          ).map((page) => (
            <Button
              key={page}
              variant={page === pageIndex ? "default" : "outline"}
              size="sm"
              onClick={() => handlePageChange(page)}
            >
              {page + 1}
            </Button>
          ))}

          {/* Next Button */}
          <Button
            variant="outline"
            size="sm"
            onClick={() => handlePageChange(pageIndex + 1)}
            disabled={pageIndex === totalPages - 1}
          >
            Next
          </Button>

          {/* Go to End Button */}
          <Button
            variant="outline"
            size="sm"
            onClick={() => handlePageChange(totalPages - 1)}
            disabled={pageIndex === totalPages - 1}
          >
            Go to End
          </Button>
        </div>
      </div>
    );
  }
}
