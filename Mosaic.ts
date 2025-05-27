import { MouseCanvasStyle } from "./MouseCanvasStyle";
import { Shape } from "./Shape";

/*
 * @Description: create by southernMD
 */
export class Mosaic {
    private mosaicSize: number;
    public canvas: HTMLCanvasElement | null = null;
    private img = new Image();
    private imgDataOriginal: Uint8ClampedArray | null = null;
    private imgDataMosaic: Uint8ClampedArray | null = null;
    private isMouseDown = false;
    private pixelSize = 5
    private isMosaicAreaSelected = false;
    private startX = 0;
    private startY = 0;

    constructor(mosaicSize: number) {
        this.mosaicSize = mosaicSize;
        this.canvas = document.createElement("canvas");
        this.canvas.style.position = "fixed";
    }

    public setImgCanvas = (imgUrl: string, clip: MouseCanvasStyle["clip"]) => {
        this.img.src = imgUrl;
        this.img.onload = () => {
            this.resizeMosaic(clip);
        };
    };

    public resizeMosaicSize = (size: number) => {
        this.mosaicSize = size;
    };

    public resizeMosaic = (clip: MouseCanvasStyle["clip"]) => {
        this.canvas!.style.left = clip.startX  + "px";
        this.canvas!.style.top = clip.startY + "px";
        const { startX, startY, endX, endY } = clip;
        const width = endX - startX ;
        const height = endY - startY;
        this.canvas!.width = width;
        this.canvas!.height = height;
        this.startX = startX;
        this.startY = startY;
        if (!document.body.contains(this.canvas)) {
            document.body.appendChild(this.canvas!);
            window!.addEventListener("mousedown", this.handleMouseDown);
            window!.addEventListener("mousemove", this.handleMouseMove);
            window!.addEventListener("mouseup", this.handleMouseUp);
        }
    };
    public updateImgData = ()=>{
        const ctx = this.canvas!.getContext("2d")!;
        ctx.drawImage(this.img, this.startX, this.startY, this.canvas!.width, this.canvas!.height, 0, 0, this.canvas!.width, this.canvas!.height);
        this.imgDataOriginal = ctx.getImageData(0, 0, this.canvas!.width, this.canvas!.height).data;
        this.imgDataMosaic = this.createMosaicData(this.canvas!.width, this.canvas!.height);
    }

    public destroy = () => {
        this.canvas?.remove();
        window!.removeEventListener("mousedown", this.handleMouseDown);
        window!.removeEventListener("mousemove", this.handleMouseMove);
        window!.removeEventListener("mouseup", this.handleMouseUp);
    };

    private handleMouseDown = (e: MouseEvent) => {
        if (e.button !== 0) return;
        if (!Shape.isMosaic || !Shape.isInCanvas(e.clientX, e.clientY)) return;
        this.isMouseDown = true;
        this.isMosaicAreaSelected = true;
    };

    private handleMouseMove = (e: MouseEvent) => {
        if (!this.isMouseDown || !this.isMosaicAreaSelected) return;
        this.renderMosaic(e.clientX, e.clientY); // 鼠标移动时继续应用马赛克
    };

    private handleMouseUp = () => {
        if (!this.isMouseDown) return;
        this.isMouseDown = false;
        this.isMosaicAreaSelected = false;
    };

    // 创建马赛克效果数据
    private createMosaicData = (width: number, height: number): Uint8ClampedArray => {
        const mosaicCanvas = document.createElement("canvas");
        mosaicCanvas.width = width;
        mosaicCanvas.height = height;

        const ctx = mosaicCanvas.getContext("2d")!;
        ctx.drawImage(this.img, 0, 0, width, height);

        const imgData = ctx.getImageData(0, 0, width, height);
        const data = imgData.data;

        // Apply mosaic effect on image data
        for (let y = 0; y < height; y += this.pixelSize) {
            for (let x = 0; x < width; x += this.pixelSize) {
                const red = this.getAverageColor(x, y, "r");
                const green = this.getAverageColor(x, y, "g");
                const blue = this.getAverageColor(x, y, "b");

                // Apply the calculated color to the pixel block
                this.fillMosaicBlock(x, y, red, green, blue, data);
            }
        }
        return data;
    };

    // 获取马赛克块的平均颜色
    private getAverageColor = (x: number, y: number, colorChannel: string) => {
        let r = 0, g = 0, b = 0;
        let count = 0;

        // 遍历马赛克块的每个像素
        for (let row = y; row < y + this.pixelSize && row < this.canvas!.height; row++) {
            for (let col = x; col < x + this.pixelSize && col < this.canvas!.width; col++) {
                const index = (row * this.canvas!.width + col) * 4;
                r += this.imgDataOriginal![index];     // Red
                g += this.imgDataOriginal![index + 1]; // Green
                b += this.imgDataOriginal![index + 2]; // Blue
                count++;
            }
        }

        // 计算平均颜色
        r = Math.floor(r / count);
        g = Math.floor(g / count);
        b = Math.floor(b / count);

        return colorChannel === "r" ? r : colorChannel === "g" ? g : b;
    };

    // 填充马赛克块
    private fillMosaicBlock = (x: number, y: number, r: number, g: number, b: number, data: Uint8ClampedArray) => {
        for (let row = y; row < y + this.pixelSize && row < this.canvas!.height; row++) {
            for (let col = x; col < x + this.pixelSize && col < this.canvas!.width; col++) {
                const index = (row * this.canvas!.width + col) * 4;
                data[index] = r;       // Red
                data[index + 1] = g;   // Green
                data[index + 2] = b;   // Blue
                data[index + 3] = 255; // Alpha
            }
        }
    };

    // 渲染选中区域
    private renderMosaic = (clientX: number, clientY: number) => {
        const ctx = this.canvas!.getContext("2d")!;
        const width = this.canvas!.width;
        const height = this.canvas!.height;
        const imgData = ctx.getImageData(0, 0, width, height);
        const data = imgData.data;

        const brushRadius = this.mosaicSize;  // 笔刷的半径
        const startX = clientX - this.canvas!.getBoundingClientRect().left;
        const startY = clientY - this.canvas!.getBoundingClientRect().top;

        // 遍历当前笔刷区域，填充相应的数据（原图或马赛克数据）
        for (let y = startY - brushRadius; y < startY + brushRadius; y += this.pixelSize) {
            for (let x = startX - brushRadius; x < startX + brushRadius; x += this.pixelSize) {
                const distance = Math.sqrt(Math.pow(x - startX, 2) + Math.pow(y - startY, 2));
                const index = (y * width + x) * 4;

                if (distance <= brushRadius) {
                    // 如果鼠标在当前区域内，应用马赛克数据
                    if (this.isMosaicAreaSelected) {
                        for (let offsetY = 0; offsetY <= this.pixelSize; offsetY++) {
                            for (let offsetX = 0; offsetX <= this.pixelSize; offsetX++) {
                                const targetX = x + offsetX;
                                const targetY = y + offsetY;
        
                                // 确保不越界
                                if (targetX >= 0 && targetX < width && targetY >= 0 && targetY < height) {
                                    const targetIndex = (targetY * width + targetX) * 4;
        
                                    // 将目标区域的像素设置为马赛克颜色
                                    data[targetIndex] = this.imgDataMosaic![targetIndex]; // Red
                                    data[targetIndex + 1] = this.imgDataMosaic![targetIndex + 1]; // Green
                                    data[targetIndex + 2] = this.imgDataMosaic![targetIndex + 2]; // Blue
                                }
                            }
                        }
                    } else {
                        // 否则应用原图数据
                        data[index] = this.imgDataOriginal![index]; // Red
                        data[index + 1] = this.imgDataOriginal![index + 1]; // Green
                        data[index + 2] = this.imgDataOriginal![index + 2]; // Blue
                    }
                }
            }
        }
        ctx.putImageData(imgData, 0, 0);
    };
}
