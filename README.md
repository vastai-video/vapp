

## VAPP


## Description
The VASTAI Performance Primitives (VAPP) library provides GPU-accelerated image processing functions.

## Function Naming

The general naming scheme is:  
vapp\<module info\>\<PrimitiveName\>_\<data-type info\>[\<additional flavor info\>](\<parameter list\>)

module info:	i,s  

PrimitiveName:	基本操作原子, 例如NV12ToRGB.  

data-type info:	8u,8s,16u, 16s,32u,32s,64u,64s,32f,64f	

additional flavor info:  

        ”A” 	如果图像是4通道的，操作不影响alpha通道，例, vappiWarpPerspectiveQuad_32s_AC4R  
        ”Cn” 	表示图像有n个packed通道，如RGB是C3，例 vppiNV12ToRGB_8u_P2C3R_Ctx  	
        ”Pn” 	图像有n个分开的平面，比如NV12是P2，YUV420P是P3，例 vppiNV12ToRGB_8u_P2C3R_Ctx 
        ”C”	    channel-of-interest，表示只操作前面所指的通道，其他通道不作用，例如vappiCopy_16s_C4CR_Ctx 
        ”I” 	in-place, 表示操作原地生效。例 vappiMirror_32f_C1IR_Ctx  			
        ”M” 	输入中有"Mask Image",只有非0像素对应的输入像素才会生效。例 vappiMean_16u_C1MR_Ctx  	
        ”R”	    region_of_interest，操作只在ROI矩形区域生效，例 vappiMean_16u_C1MR_Ctx	  		
        ”Sfs”	表示图像输出前，像素会经过固定的缩放和饱和处理，例 vappiRectStdDev_32s_C1RSfs_Ctx  
     

## Function List
vappiYUV420Resize_8u_P3

vappiResize_8u_P2

vappiColorToGray_8u_C3C1

vappiResize_8u_C3

vappiResize_8u_C3_Ctx

vappiCrop_8u_C3

## Installation
./compile.sh

## Usage
source env.sh
1. rgb24 async resize  
./bin/test --inputfile /home/vastai/simonz/input/Winter_8K_8bit_1000.rgb --outputfile out.rgb --input_size 7680x4320 --output_size 2000x3000 --vframes 1 --test_case 1

2. rgb24 resize  
./bin/test --inputfile /home/vastai/simonz/jzztd_10.rgb --outputfile out.rgb --input_size 1920x1080 --output_size 500x600 --vframes 1 --test_case 2

3. yuv420 resize
./bin/test --inputfile /home/vastai/simonz/jzztd_yuv420p_100.yuv --outputfile out.yuv --input_size 1920x1080 --output_size 500x600 --vframes 1 --test_case 3

4. nv12 ctx resize  
./bin/test --inputfile /home/vastai/simonz/input/jzztd_nv12_100.yuv --outputfile out.yuv --input_size 1920x1080 --output_size 500x600 --vframes 1 --test_case 4

5. yuv420 flip  
#./bin/test --inputfile /home/vastai/simonz/input/jzztd_yuv420p_100.yuv --outputfile out.yuv --input_size 1920x1080 --vframes 1 --test_case 5

6. nv12 flip
./bin/test --inputfile /home/vastai/simonz/input/jzztd_nv12_100.yuv --outputfile out.yuv --input_size 1920x1080 --vframes 1 --test_case 6  

7. rgb24 crop  
./bin/test --inputfile /home/vastai/simonz/input/jzztd_10.rgb --outputfile out.rgb --input_size 1920x1080 --output_size 500x500 --vframes 1 --test_case 7

8. yuv420 rotate  
./bin/test --inputfile /home/vastai/simonz/input/jzztd_yuv420p_100.yuv --outputfile out.yuv --input_size 1920x1080 --vframes 1 --test_case 8

9. rgb planar roi flip    
./bin/test --inputfile /home/vastai/simonz/input/Winter_8K_8bit_1000.rgbp --outputfile out_planar.rgb --input_size 7680x4320   --vframes 1 --test_case 9

10. rgb planar remap    
./bin/test --inputfile remap_c_3_h_2106_w_2704_out_dsp.bin --outputfile out_planar.rgb --input_size 2704x2106   --vframes 1 --test_case 10

11. rgb24 roi crop  
./bin/test --inputfile /home/vastai/simonz/input/jzztd_10.rgb --outputfile out.rgb --input_size 1920x1080 --output_size 500x500 --vframes 1 --test_case 11

12. rgbp roi crop  
./bin/test --inputfile /home/vastai/simonz/input/jzztd_10.rgb --outputfile out.rgb --input_size 1920x1080 --output_size 500x500 --vframes 1 --test_case 12

13. rgbp roi warpperspective  
./bin/test --inputfile /home/vastai/simonz/input/warp_perspective_u8_c_3_h_2106_w_2704_in.bin --outputfile out.rgb --input_size 2704x2106 --output_size 2720x2125 --vframes 1 --test_case 13

14. rgbp resize  
./bin/test --inputfile planar.rgb --outputfile out_planar.rgb --input_size 1920x1080 --output_size 500x600 --vframes 1 --test_case 14

15. gray remap    
./bin/test --inputfile input_gray.rgb --outputfile remap_out.rgb --input_size 2576x2032 --output_size 2704x2106  --vframes 1 --test_case 15

16. gray crop  
./bin/test --inputfile input_gray.rgb  --outputfile crop_out.rgb --input_size 2576x2032 --output_size 2400x2000 --vframes 1 --test_case 16

17. gray resize  
./bin/test --inputfile input_gray.rgb  --outputfile resize_out.rgb --input_size 2576x2032 --output_size 2000x1800 --vframes 1 --test_case 17

18. gray fixed remap    
./bin/test --inputfile input_gray.rgb --outputfile remap_out.rgb --input_size 2576x2032 --output_size 2704x2106  --vframes 1 --test_case 18

19. gray roi flip    
./bin/test --inputfile input_gray.rgb --outputfile flip_out.rgb --input_size 2576x2032 --vframes 1 --test_case 19

20. gray warpperspective    
./bin/test --inputfile input_gray.rgb --outputfile warp_out.rgb --input_size 2576x2032 --output_size 2704x2106 --config_file ./test/config.txt --vframes 1 --test_case 20

21. gray translate transform  
./bin/test --inputfile input_gray.rgb --outputfile translate_out.rgb --input_size 2576x2032 --vframes 1 --test_case 21

22. gray transpose  
./bin/test --inputfile input_gray.rgb --outputfile transpose_out.rgb --input_size 2576x2032 --vframes 1 --test_case 22

23. yuv420 eq 
./bin/test --inputfile src-cif.yuv --outputfile out.yuv --input_size 352x288 --output_size 352x288 --vframes 1 --brightness 1.3 --saturation 1.4 --contrast 2.4  --test_case 23

24. rgbp resizeplus resize include crop
./bin/test --inputfile OrgA_planar_2576x2032.rgb --outputfile out_resizeplus_OrgA_planar_400x500.rgb --input_size 2576x2032 --output_size 400x500 --vframes 1 --test_case 24

25. gray resizeplus resize include crop
./bin/test --inputfile OrgA_gray_2704x2106.rgb --outputfile out_resizeplus_OrgA_planar_400x500.rgb --input_size 2704x2106 --output_size 400x500 --vframes 1 --test_case 25

26. yuv420 ctx  resize   
./build/test/test --inputfile /home/vastai/simonz/jzztd_yuv420p_100.yuv --outputfile out.yuv --input_size 1920x1080 --output_size 500x600 --vframes 1 --test_case 26  
./build/test/test --inputfile /home/vastai/simonz/input/jzztd_yuv420p_100.yuv --outputfile out.yuv --input_size 1920x1080 --output_size 500x600 --vframes 1 --test_case 26 --elf_file /opt/vastai/vaststream/lib/op/ext_op/video/scale_ext_op  

27. yuv420 sad
./bin/test --inputfile "/orig.yuv trans.yuv" --outputfile out.yuv --input_size 1920x1080 --output_size 1920x1080 --vframes 1  --test_case 27

28. nv12 ctx rotate
./bin/test --inputfile /home/vastai/simonz/input/jzztd_yuv420p_100.yuv --outputfile out.yuv --input_size 1920x1080 --output_size 1920x1080 --input_pitch 2048x1088 --output_pitch 1920x1088 --vframes 1 --test_case 28

29. single core rgba interleaved rotate 8bit degree:90,180,270
./bin/test --inputfile rgba.rgb --outputfile out.rgb --input_size 736x1600 --vframes 1 --test_case 29 --degree 270

30. multi core rgba interleaved rotate 8bit degree:90,180,270
./bin/test --inputfile rgba.rgb --outputfile out.rgb --input_size 736x1600 --vframes 1 --test_case 30 --degree 270

31. yuv420 detection
./bin/test --inputfile "/orig.yuv" --outputfile out.roi --input_size 1920x1080 --vframes 1  --test_case 31

32. yuv420 transpose
./bin/test --inputfile yuv420p_720x1280.yuv --outputfile out_transpose_yuv420p_720x1280_3.yuv --input_size 720x1280 --vframes 1 --test_case 32

33. nv12 transpose
./bin/test --inputfile nv12_1080x1920.yuv --outputfile out_transpose_nv12_1080x1920_0.yuv --input_size 1080x1920 --vframes 1 --test_case 33

34. yuv420 cropscale not support

35. nv12 cropscale
./vapp/test/test --inputfile nv12_1080x1920.yuv --outputfile out_cropscale_nv12_540x960.yuv --input_size 1080x1920 --output_size 540x960 --vframes 1 --test_case 35

36. rgba convet nv12
./bin/test --inputfile /home/vastai/simonz/input/jzztd_1920_1080.rgb --outputfile out.yuv --input_size 1920x1080 --output_size 1920x1080 --vframes 1 --test_case 36

37. rgba scale + convert nv12
./bin/test --inputfile /home/vastai/simonz/input/jzztd_1920_1080.rgb --outputfile out.yuv --input_size 1920x1080 --output_size 1280x720  --vframes 1 --test_case 37

100. nv12 csc  
./bin/test --inputfile /home/vastai/simonz/input/jzztd_nv12_100.yuv --outputfile out.yuv --input_size 1920x1080 --vframes 1 --test_case 100 

101. rgb24 colortogray    
./bin/test --inputfile /home/vastai/simonz/jzztd_10.rgb --outputfile out.yuv --input_size 1920x1080 --vframes 1 --test_case 101

102. bayertonv12  
./bin/test --inputfile /home/vastai/simonz/input/p1_BayerGB_4096x3000.bin --outputfile out.yuv --input_size 4096x3000   --vframes 1 --test_case 102

103. rgbp colortogray    
./bin/test --inputfile /home/vastai/simonz/jzztd_10.rgb --outputfile out.yuv --input_size 1920x1080 --vframes 1 --test_case 103

104. rgbp to rgb888    
#./bin/test --inputfile out_planar.rgb --outputfile out24.rgb --input_size 7680x4320 --vframes 1 --test_case 104

200. nv12 overlay  
./bin/test --inputfile /home/vastai/simonz/input/jzztd_nv12_100.yuv --outputfile out.yuv --input_size 1920x1080 --layer_size 200x200 --vframes 1 --test_case 200

201. nv12 bitdepthcvt  
./bin/test --inputfile /home/vastai/simonz/input/jellyfish_720p_p10le_100.yuv --outputfile out.yuv --input_size 1280x720 --vframes 1 --test_case 201

202. adaptive threshold  
./bin/test --inputfile input_gray.rgb --outputfile adaptive.rgb --input_size 2560x2048 --vframes 1 --test_case 202

203. yuv unshap   
./bin/test --inputfile /home/vastai/simonz/input/jzztd_yuv420p_100.yuv --outputfile out.yuv --input_size 1920x1080 --vframes 1 --test_case 203

204. yuv hqdn3d
 ./bin/test --inputfile jzztd_yuv420p_100.yuv --outputfile out11.yuv --input_size 1920x1080 --vframes 150 --test_case 204

205. cas rgb_planar yuv420p nv12  
./bin/test --inputfile planar_2576x2032.rgb --outputfile out.rgb --input_size 2576x2032 --vframes 10 --test_case 205
