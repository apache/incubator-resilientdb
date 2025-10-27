<!--
Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.    
-->

<script lang="ts" module>
	import { type VariantProps, tv } from "tailwind-variants";

	export const badgeVariants = tv({
		base: "focus:ring-ring inline-flex items-center rounded-full border px-2.5 py-0.5 text-xs font-semibold transition-colors focus:outline-none focus:ring-2 focus:ring-offset-2",
		variants: {
			variant: {
				default:
					"bg-primary text-primary-foreground hover:bg-primary/80 border-transparent",
				secondary:
					"bg-secondary text-secondary-foreground hover:bg-secondary/80 border-transparent",
				destructive:
					"bg-destructive text-destructive-foreground hover:bg-destructive/80 border-transparent",
				outline: "text-foreground",
			},
		},
		defaultVariants: {
			variant: "default",
		},
	});

	export type BadgeVariant = VariantProps<typeof badgeVariants>["variant"];
</script>

<script lang="ts">
	import type { WithElementRef } from "bits-ui";
	import type { HTMLAnchorAttributes } from "svelte/elements";
	import { cn } from "$lib/utils.js";

	let {
		ref = $bindable(null),
		href,
		class: className,
		variant = "default",
		children,
		...restProps
	}: WithElementRef<HTMLAnchorAttributes> & {
		variant?: BadgeVariant;
	} = $props();
</script>

<svelte:element
	this={href ? "a" : "span"}
	bind:this={ref}
	{href}
	class={cn(badgeVariants({ variant, className }))}
	{...restProps}
>
	{@render children?.()}
</svelte:element>
