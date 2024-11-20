"use client"
import { cn } from "@/lib/utils"
import { BookOpen, MenuIcon } from 'lucide-react'
import Link from "next/link"
import * as React from "react"
import { Dialog, DialogClose } from "@/components/ui/dialog";
import { Button } from "@/components/ui/button";
import { NavigationMenu, NavigationMenuLink, NavigationMenuList } from "@/components/ui/navigation-menu";
import ModeToggle from "../mode-toggle"
import { SheetContent, SheetDescription, SheetHeader, SheetTitle, SheetTrigger } from "@/components/ui/sheet"
// import ModeToggle from "./mode-toggle"


export function NavBar() {

    return (
        <div className="flex items-center min-w-full w-full fixed justify-center p-2 z-[50] mt-[2rem]">
            <div className="flex justify-between md:w-[720px] w-[95%] border dark:border-zinc-900 dark:bg-black bg-opacity-10 relative backdrop-filter backdrop-blur-lg bg-white border-white border-opacity-20 rounded-xl p-2 shadow-lg">
                <Dialog>
                    <SheetTrigger className="min-[825px]:hidden p-2 transition">
                        <MenuIcon />
                    </SheetTrigger>
                    <SheetContent side="left">
                        <SheetHeader>
                            <SheetTitle>MemLens.</SheetTitle>
                            <SheetDescription>
                                Scale and launch products with expert developers, on-demand, at a flat monthly fee
                            </SheetDescription>
                        </SheetHeader>
                        <div className="flex flex-col space-y-3 mt-[1rem] z-[99]">
                            <DialogClose asChild>
                                <Link href="/">
                                    <Button variant="outline" className="w-full">Home</Button>
                                </Link>
                            </DialogClose>
                            <DialogClose asChild>
                                <Link href="/software">
                                    <Button variant="outline" className="w-full">Software</Button>
                                </Link>
                            </DialogClose>
                            <DialogClose asChild>
                                <Link href="/automation">
                                    <Button variant="outline" className="w-full">Automation</Button>
                                </Link>
                            </DialogClose>
                            <DialogClose asChild>
                                <Link href="/blog">
                                    <Button variant="outline" className="w-full">Blog</Button>
                                </Link>
                            </DialogClose>
                            <DialogClose asChild>
                                <Link href="/projects">
                                    <Button variant="outline" className="w-full">Projects</Button>
                                </Link>
                            </DialogClose>
                            <ModeToggle />
                        </div>
                    </SheetContent>
                </Dialog>
                <NavigationMenu>
                    <NavigationMenuList className="max-[825px]:hidden ">
                        <Link href="/" className="pl-2">
                            <h1 className="font-bold">fabrika.</h1>
                        </Link>
                    </NavigationMenuList>
                </NavigationMenu>
                <div className="flex items-center gap-2 max-[825px]:hidden">
                    <Link href="/software">
                        <Button variant="ghost">Software</Button>
                    </Link>
                    <Link href="/automation">
                        <Button variant="ghost">Automation</Button>
                    </Link>
                    <Link href="/blog">
                        <Button variant="ghost">Blog</Button>
                    </Link>
                    <Link href="/projects">
                        <Button variant="ghost">Projects</Button>
                    </Link>
                    <ModeToggle />
                </div>
            </div>
        </div>

    )
}

const ListItem = React.forwardRef<
    React.ElementRef<"a">,
    React.ComponentPropsWithoutRef<"a">
>(({ className, title, children, ...props }, ref) => {
    return (
        <li>
            <NavigationMenuLink asChild>
                <a
                    ref={ref}
                    className={cn(
                        "block select-none space-y-1 rounded-md p-3 leading-none no-underline outline-none transition-colors hover:bg-accent hover:text-accent-foreground focus:bg-accent focus:text-accent-foreground",
                        className
                    )}
                    {...props}
                >
                    <div className="text-sm font-medium leading-none">{title}</div>
                    <p className="line-clamp-2 text-sm leading-snug text-muted-foreground">
                        {children}
                    </p>
                </a>
            </NavigationMenuLink>
        </li>
    )
})
ListItem.displayName = "ListItem"
