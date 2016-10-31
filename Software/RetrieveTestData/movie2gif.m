function movie2gif(F, filename)
    for i = 1:numel(F)
        im{i} = frame2im(F(i));
        [A,map] = rgb2ind(im{i},256);
        if i == 1
            imwrite(A,map,filename,'gif','LoopCount',Inf,'DelayTime',.25);
        else
            imwrite(A,map,filename,'gif','WriteMode','append','DelayTime',.25);
        end
    end
    winopen(filename)
end