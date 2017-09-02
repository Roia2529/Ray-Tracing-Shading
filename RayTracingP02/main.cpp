//
//  main.cpp
//  RayTracingP02
//
//  Created by Hsuan Lee on 8/30/17.
//  Copyright © 2017 Hsuan Lee. All rights reserved.
//

#include <iostream>
#include <thread>
#include "viewport.cpp"
#include "xmlload.cpp"
#include "objects.h"
#include <math.h>
#include <algorithm> 

using namespace std;

Node rootNode;
Camera camera;
RenderImage renderImage;
Sphere theSphere;
MaterialList materials;
LightList lights;

class pixelIterator{
private:
    atomic<int> ix;
public:
    void Init(){ ix = 0;}
    bool GetPixel(int &x, int &y){
        int i=ix++;
        if(i>=camera.imgWidth*camera.imgHeight) return false;
        x = i%camera.imgWidth;
        y = i/camera.imgWidth;
        return true;
    }
};
//multi-thread
pixelIterator pIt;

bool Trace(const Ray &ray, HitInfo &hitinfo);
bool TraceNode(const Node &node, const Ray &ray, HitInfo &hitinfo);
void setPixelblack(int index);
void setPixelwhite(int index, const HitInfo hinfo);
void RenderPixel(pixelIterator &it);

void RenderPixel(pixelIterator &it){
    int x,y;
    
    float theta = camera.fov;
    float l = 1.0;
    float h = 2*l*tan(theta/2*(M_PI/180));
    float w = h*(float)camera.imgWidth/camera.imgHeight;
    Point3 b;
    b.Set(-w/2,h/2,-l);
    float u = w/camera.imgWidth;
    float v = -h/camera.imgHeight;
    b.x+=u/2;
    b.y+=v/2;
    
    Point3 z_new = -1*(camera.dir);
    Point3 x_new = camera.up ^ z_new;
    camera.up.Normalize();
    z_new.Normalize();
    x_new.Normalize();
    Matrix3 m(x_new,camera.up, z_new);
    
    while(it.GetPixel(x,y)){
        Point3 tmp(x*u,y*v,0);
        tmp+=b;
        Ray ray_pixel(camera.pos,tmp);
        ray_pixel.dir=m*(ray_pixel.dir);
        ray_pixel.dir.Normalize();
        
        HitInfo hitinfo;
        hitinfo.Init();
        
        if(rootNode.GetNumChild()>0){
            //Color24* color_pixel = renderImage.GetPixels();
            
            if(Trace(ray_pixel,hitinfo)){
                //set color
                setPixelwhite(y*camera.imgWidth+x,hitinfo);
            }
            else{
                setPixelblack(y*camera.imgWidth+x);
            }
            renderImage.IncrementNumRenderPixel(1);
        }
    }
}

Color MtlBlinn::Shade(const Ray &ray, const HitInfo &hInfo, const LightList &lights) const{
    return Color(255.0);
}

bool Sphere::IntersectRay( const Ray &ray, HitInfo &hitinfo, int hitSide ) const{
	//float raidus = 1;
	float a = ray.dir.Dot(ray.dir);
	float c = ray.p.Dot(ray.p)-1;
	float b = 2*ray.p.Dot(ray.dir);
	float insqrt = b*b-(4*a*c);
	if(insqrt>=0){
		float t1 = (-b+sqrtf(insqrt))/(a*2);
		float t2 = (-b-sqrtf(insqrt))/(a*2);
		float prez = hitinfo.z;
        
        hitinfo.z = min(t1,t2);
        if(hitinfo.z<0)
        	return false;
        else if(hitinfo.z < prez)
         	return true;
        else hitinfo.z=prez;
        
        hitinfo.p = hitinfo.z*ray.dir+ray.p;
        hitinfo.N = hitinfo.p;
	}

	return false;
}

bool Trace(const Ray &ray, HitInfo &hitinfo){
    return TraceNode(rootNode, ray, hitinfo);
}

bool TraceNode(const Node &node,const Ray &ray, HitInfo &hitinfo){
	//world space to model space
	Ray r = node.ToNodeCoords(ray);
	const Object *obj = node.GetNodeObj();
	bool hit = false;

	if(obj){
		if(obj->IntersectRay(r,hitinfo)){
			hit = true;
			hitinfo.node = &node;
		}
	}
	for(int i=0;i<node.GetNumChild();i++){
		const Node &child = *node.GetChild(i);
		if(TraceNode(child,r,hitinfo)){
			hit = true;
		}
	}
    return hit;
}

void setPixelblack(int index){
	Color24* color_pixel = renderImage.GetPixels();
    (color_pixel+ index)->r=0;
    (color_pixel+ index)->g=0;
    (color_pixel+ index)->b=0;
    float* zb_pixel = renderImage.GetZBuffer();
    //cout<< "index: "<<index<<"\n";
    zb_pixel[index] = BIGFLOAT;
    		
}	

void setPixelwhite(int index, const HitInfo hinfo){
	Color24* color_pixel = renderImage.GetPixels();
    (color_pixel+ index)->r=255;
    (color_pixel+ index)->g=255;
    (color_pixel+ index)->b=255;
    
    float* zb_pixel = renderImage.GetZBuffer();
    *(zb_pixel+ index) = hinfo.z;
    	
}
void BeginRender()
{	
	cout<<"call by GlutKeyboard() in viewport.cpp\n";
	
    unsigned num_thread = thread::hardware_concurrency();
    vector<thread> thr;
    for(int j=0;j<num_thread;j++){
        thread th(RenderPixel,ref(pIt));
        thr.push_back(move(th));
    }
    
    //join
    for(int j=0;j<num_thread;j++){
        thr.at(j).join();
    }
    
    cout << "Saving z-buffer image...\n";
    renderImage.ComputeZBufferImage();
    renderImage.SaveZImage("/Users/hsuanlee/Documents/Cpp/RayTracingP02/RayTracingP02/prj2.png");
}

void StopRender(){
    //stop multithread
    
}


int main(int argc, const char * argv[]) {
    pIt.Init();
    //const char *file = "simplescene.xml"; //can't load the file
    const char *file = "/Users/hsuanlee/Documents/Cpp/RayTracingP02/RayTracingP02/input.xml";
    LoadScene(file);
    ShowViewport();
    
    return 0;
}
