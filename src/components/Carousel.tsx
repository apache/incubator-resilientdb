import React, { useState } from 'react';
import Slider from 'react-slick';
import { ChevronLeft, ChevronRight } from 'lucide-react';
import 'slick-carousel/slick/slick.css';
import 'slick-carousel/slick/slick-theme.css';

interface CarouselProps {
  children: React.ReactNode[];
}

const PrevArrow = (props: any) => {
  const { onClick } = props;
  return (
    <button
      onClick={onClick}
      className="absolute left-4 top-1/2 z-10 -translate-y-1/2 bg-gray-800 p-2 rounded-full transition-opacity duration-300 hover:bg-gray-700"
      aria-label="Previous slide"
    >
      <ChevronLeft className="w-6 h-6 text-white" />
    </button>
  );
};

const NextArrow = (props: any) => {
  const { onClick } = props;
  return (
    <button
      onClick={onClick}
      className="absolute right-4 top-1/2 z-10 -translate-y-1/2 bg-gray-800 p-2 rounded-full transition-opacity duration-300 hover:bg-gray-700"
      aria-label="Next slide"
    >
      <ChevronRight className="w-6 h-6 text-white" />
    </button>
  );
};

export const Carousel: React.FC<CarouselProps> = ({ children }) => {
  const [currentSlide, setCurrentSlide] = useState(0);

  const settings = {
    dots: false,
    infinite: true,
    speed: 500,
    slidesToShow: 1,
    slidesToScroll: 1,
    prevArrow: <PrevArrow />,
    nextArrow: <NextArrow />,
    beforeChange: (current: number, next: number) => {
      console.log(`Changing from slide ${current} to ${next}`);
      setCurrentSlide(next);
    },
    afterChange: (current: number) => {
      console.log(`Changed to slide ${current}`);
    },
  };

  return (
    <div className="relative overflow-hidden">
      <Slider {...settings}>
        {React.Children.map(children, (child, index) => (
          <div key={index} className="px-4">
            {child}
          </div>
        ))}
      </Slider>
      <div className="mt-4 text-center text-white">
        Current Slide: {currentSlide + 1} / {React.Children.count(children)}
      </div>
    </div>
  );
};

