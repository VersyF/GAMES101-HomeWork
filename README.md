# GAMES101-HW
Records of homework from GAMES101

= Homework 0 =  

26/9/29 - complete homework0
![alt text](MarkdownPicture/Snipaste_2026-09-29_19-21-41.png)

= Homework 1 =

26/9/30 - accomplish Model Matrix Part (rotate around z-axis)

26/10/01 - complete homework1 (projection matrix + rotate around more axis)

![alt text](MarkdownPicture/Oct01-3-z2.gif)
![alt text](MarkdownPicture/Oct01-1-x.gif)
![alt text](MarkdownPicture/Oct01-2-y.gif)

= Homework 2 =

26/10/2 - accomplish rasterize_Triangle function without check

26/10/3 - fixed some bugs

26/10/6 - complete homework2
![alt text](MarkdownPicture/Snipaste_2026-10-06_22-42-00.png)

= Homework 3 =

26/10/9 - accomplish interpolation part; fixed a few bugs  
(interpolated normal output)
![alt text](MarkdownPicture/output.png)

26/10/11 - accomplish Phong part，未验证

26/10/12   
调试并修复phong shader  
![alt text](MarkdownPicture/phong.png)

实现、调试、修复texture Shader  
![alt text](MarkdownPicture/texture2.png)  
normal插值后未重新归一化的结果：  
![alt text](MarkdownPicture/texture.png)

实现、调试、修复bump shader  
![alt text](MarkdownPicture/bump.png)

实现、调试、修复displacement shader
![alt text](MarkdownPicture/displacement2.png)  
未移动position效果：  
![alt text](MarkdownPicture/displacement.png)

Homework 3 completed

= Homework 4 =

26/10/13  
实现、调试、修复homework4  
![alt text](MarkdownPicture/combine.png)

MSAA：创建antialiasMSAA，传入曲线点集，创建包围盒，进行MSAA反走样  
![alt text](MarkdownPicture/MSAA.png)  
反走样优化：取消包围盒，改为标记 曲线点集 中每个点附近3*3的格子，只对标记的格子进行MSAA反走样  
对于斜线的渲染速度，从原先10s左右提升到1秒内  

Homework 4 completed

= Homework 5 =

26/10/14  
HW5:实现Render的发射坐标计算，未调试

26/10/15  
HW5：实现Triangle的射线求交  
![alt text](MarkdownPicture/binary.png)

Homework 5 completed

= Homework 6 =

26/10/16  
HW6：实现Renderer射线发射，未验证

26/10/17  
HW6：  
实现三角形求交intersection设置，未验证    
实现intersect包围盒射线碰撞判断，未验证  
实现BVH getIntersection  
完成调试、修复bug  
![alt text](MarkdownPicture/1.png)
修复三角形intersection.coords计算错误  
![alt text](MarkdownPicture/binary-1.png)

Homework 6 completed

= Homework 7 =

26/10/18  
HW7：从HW6迁移已有部分代码  
HW7：Review HW6 Scene.castRay

26/10/20  
HW7：编写直接光部分，存在阴影遮挡判断问题？，右侧小方块全黑  
![alt text](MarkdownPicture/direct(ShadowCast3).png)

26/10/21
HW7：添加间接光，存在bug  
![alt text](MarkdownPicture/binary-2.png)