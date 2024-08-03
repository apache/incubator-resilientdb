export { Verdana110 as calcWidth } from './calc-text-width';
declare type StyleOption = 'flat' | 'classic';
interface BadgenOptions {
    status: string;
    subject?: string;
    color?: string;
    label?: string;
    labelColor?: string;
    style?: StyleOption;
    icon?: string;
    iconWidth?: number;
    scale?: number;
}
export declare function badgen({ label, subject, status, color, style, icon, iconWidth, labelColor, scale }: BadgenOptions): string;
declare global {
    interface Window {
        badgen: typeof badgen;
    }
}
